/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2015 Xuefer <xuefer@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "lights"
#include <cutils/log.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hardware/lights.h>
#include <algorithm>

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static light_state_t g_attention;
static light_state_t g_notification;
static light_state_t g_battery;
static light_state_t g_speaker_light;
static int g_buttons_brightness = -1;

static char const *const LCD_FILE  = "/sys/class/leds/lcd-backlight/brightness";
static char const *const BUTTON_FILE = "/sys/class/leds/button-backlight/brightness";
static char const *const FLASHLIGHT_LED = "/sys/class/leds/flashlight/brightness";

static int read_buffer(char const *path, char *buffer, size_t buffer_size)
{
	static bool already_warned = false;

	int fd = -1;
	int err = 0;
	ssize_t bytes;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		err = -errno;
		goto error;
	}

	bytes = read(fd, buffer, buffer_size - 1);
	if (bytes == -1) {
		err = -errno;
		goto error;
	}

	buffer[bytes] = '\0';
	ALOGV("read_buffer: path %s, buffer %s", path, buffer);

	close(fd);
	return 0;

error:
	if (fd != -1) {
		close(fd);
	}

	if (!already_warned) {
		ALOGE("read_buffer failed to open %s: %d\n", path, err);
		already_warned = true;
	}

	return err;
}

static int read_int(char const *path, int default_value)
{
	int value = default_value;
	int fd;
	static int already_warned = 0;

	fd = open(path, O_RDONLY);
	if (fd >= 0) {
		char buffer[20];
		read(fd, buffer, sizeof(buffer) - 1);
		buffer[sizeof(buffer) - 1] = '\0';
		close(fd);

		value = atoi(buffer);
		ALOGV("read_int: path %s, value %d", path, value);
	} else {
		if (already_warned == 0) {
			ALOGE("read_int failed to open %s\n", path);
			already_warned = 1;
		}
	}
	return value;
}

static int write_int(char const *path, int value)
{
	int fd;
	static int already_warned = 0;

	ALOGV("write_int: path %s, value %d", path, value);
	fd = open(path, O_WRONLY);
	if (fd >= 0) {
		char buffer[20];
		int bytes = sprintf(buffer, "%d\n", value);
		int amt = write(fd, buffer, bytes);
		close(fd);
		return amt == -1 ? -errno : 0;
	} else {
		if (already_warned == 0) {
			ALOGE("write_int failed to open %s\n", path);
			already_warned = 1;
		}
		return -errno;
	}
}

static int write_int(char const *path, int i1, int i2, int i3, int i4)
{
	int fd;
	static int already_warned = 0;

	ALOGV("write_int: path %s, value %d %d %d %d", path, i1, i2, i3, i4);
	fd = open(path, O_WRONLY);
	if (fd >= 0) {
		char buffer[20 * 4];
		int bytes = sprintf(buffer, "%d %d %d %d\n", i1, i2, i3, i4);
		int amt = write(fd, buffer, bytes);
		close(fd);
		return amt == -1 ? -errno : 0;
	} else {
		if (already_warned == 0) {
			ALOGE("write_int failed to open %s\n", path);
			already_warned = 1;
		}
		return -errno;
	}
}

static int is_lit(light_state_t const &state)
{
	return state.color & 0x00ffffff;
}

static int rgb_to_brightness(light_state_t const &state)
{
	int color = state.color & 0x00ffffff;
	int alpha = (state.color>>24) & 0xff;
	if (!alpha) {
		alpha = 0xff;
	}

	return (((77*((color>>16) & 0x00ff))
			+ (150*((color>>8) & 0x00ff)) + (29*(color & 0x00ff))) >> 8) * alpha / 0xFF;
}

static int rgb_to_brightness_max(light_state_t const &state)
{
	int color = state.color & 0x00ffffff;
	int alpha = (state.color>>24) & 0xff;
	if (!alpha) {
		alpha = 0xff;
	}

	int r = (color>>16) & 0x00ff;
	int g = (color>>8) & 0x00ff;
	int b = color & 0x00ff;

	return std::max(std::max(r, g), b) * alpha / 0xFF;
}

static void dump_light_state(char const* name, light_state_t const &state)
{
	ALOGV("%s: color=%06X, mode=%d onMs=%d offMs=%d brightnessMode=%d ledsModes=%d"
		  , name
		  , state.color
		  , state.flashMode
		  , state.flashOnMS
		  , state.flashOffMS
		  , state.brightnessMode
		  , state.ledsModes
		  );
}

__BEGIN_DECLS

static int set_light_backlight(light_device_t* dev, light_state_t const* state)
{
	int err = 0;
	int brightness = rgb_to_brightness_max(*state);
	if (brightness) {
		brightness = (brightness + 1) * 16 - 1;
	}
	static int old_brightness = -1;

	pthread_mutex_lock(&g_lock);
	if (old_brightness != brightness) {
		err = write_int(LCD_FILE, brightness);
		old_brightness = brightness;
	}
	pthread_mutex_unlock(&g_lock);

	return err;
}

__END_DECLS

// 0 ~ 7
static int msToLedTime(int ms)
{
	if (ms <=     1) return 255;
	if (ms <=   250) return 0;
	if (ms <=   500) return 1;
	if (ms <=  1000) return 2;
	if (ms <=  2500) return 3;
	if (ms <=  5000) return 4;
	if (ms <=  8000) return 5;
	if (ms <= 15000) return 6;
	return 7;
}

static int stateToLedTime(light_state_t const &state, bool forOn)
{
	switch (state.flashMode) {
	case LIGHT_FLASH_NONE:
		return 255;

	default:
		return msToLedTime(forOn ? state.flashOnMS : state.flashOffMS);
	}
}

static int set_speaker_light_locked(light_device_t* dev, light_state_t const* state)
{
	dump_light_state("attention",    g_attention);
	dump_light_state("notification", g_notification);
	dump_light_state("battery",      g_battery);

	if (g_speaker_light.color == state->color
	 && g_speaker_light.flashMode == state->flashMode
	 && g_speaker_light.flashOnMS == state->flashOnMS
	 && g_speaker_light.flashOffMS == state->flashOffMS) {
		ALOGV("speaker light not changed");
		return 0;
	}
	g_speaker_light = *state;

	if (is_lit(g_speaker_light)) {
		if (g_speaker_light.flashMode == LIGHT_FLASH_NONE) {
			write_int("/sys/class/leds/white/blink", 0);
		}
		else {
			int onTime = msToLedTime(g_speaker_light.flashOnMS);
			int offTime = msToLedTime(g_speaker_light.flashOffMS);
			write_int("/sys/class/leds/white/led_time", onTime, onTime / 2, offTime, offTime / 2);
			write_int("/sys/class/leds/white/blink", 1);
		}

		write_int("/sys/class/leds/white/brightness", rgb_to_brightness(g_speaker_light));
	} else {
		write_int("/sys/class/leds/white/brightness", 0);
		write_int("/sys/class/leds/white/blink", 0);
	}

	return 0;
}

static int update_speaker_light_locked(light_device_t* dev)
{
	if (!(g_attention.ledsModes & LIGHT_MODE_MULTIPLE_LEDS) && is_lit(g_attention)) {
		return set_speaker_light_locked(dev, &g_attention);
	} else if (is_lit(g_notification)) {
		return set_speaker_light_locked(dev, &g_notification);
	} else {
		return set_speaker_light_locked(dev, &g_battery);
	}
}

__BEGIN_DECLS

static int set_light_attention(light_device_t* dev, light_state_t const* state)
{
	ALOGV(__func__);
	pthread_mutex_lock(&g_lock);
	g_attention = *state;
	int ret = 0;
	if (!(g_attention.ledsModes & LIGHT_MODE_MULTIPLE_LEDS)) {
		ret = update_speaker_light_locked(dev);
	}
	else {
		dump_light_state("attention", g_attention);

		// notify attention thread
		int brightness = rgb_to_brightness(g_attention);
		static int old_brightness = -1;
		static bool using_flashlight = false;

		if (old_brightness != brightness) {
			if (using_flashlight || read_int(FLASHLIGHT_LED, 0) == 0) {
				ret = write_int(FLASHLIGHT_LED, brightness);
				using_flashlight = brightness != 0;
			}
			old_brightness = brightness;
		}
	}
	pthread_mutex_unlock(&g_lock);
	return ret;
}

static int set_light_notifications(light_device_t* dev, light_state_t const* state)
{
	ALOGV(__func__);
	pthread_mutex_lock(&g_lock);
	g_notification = *state;
	int ret = update_speaker_light_locked(dev);
	pthread_mutex_unlock(&g_lock);
	return ret;
}

static int set_light_battery(light_device_t* dev, light_state_t const* state)
{
	ALOGV(__func__);
	pthread_mutex_lock(&g_lock);
	g_battery = *state;
	int ret = update_speaker_light_locked(dev);
	pthread_mutex_unlock(&g_lock);
	return ret;
}

static int set_light_buttons(light_device_t* dev, light_state_t const* state)
{
	int err = 0;
	int brightness = rgb_to_brightness(*state);

	pthread_mutex_lock(&g_lock);
	if (g_buttons_brightness != brightness) {
		g_buttons_brightness = brightness;
		err = write_int(BUTTON_FILE, g_buttons_brightness);
	}
	pthread_mutex_unlock(&g_lock);

	return err;
}

static int close_lights(light_device_t *dev)
{
	if (dev) {
		delete dev;
	}
	return 0;
}


/** Open a new instance of a lights device using name */
static int open_lights(const hw_module_t *module, char const *name,
						hw_device_t **device)
{
	int (*set_light)(light_device_t *dev, light_state_t const *state);

	ALOGV("open_lights: open with %s", name);

	if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
		set_light = set_light_backlight;
	else if (0 == strcmp(LIGHT_ID_BUTTONS, name))
		set_light = set_light_buttons;
	else if (0 == strcmp(LIGHT_ID_ATTENTION, name))
		set_light = set_light_attention;
	else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
		set_light = set_light_notifications;
	else if (0 == strcmp(LIGHT_ID_BATTERY, name))
		set_light = set_light_battery;
	else
		return -EINVAL;

	auto dev = new light_device_t;
	memset(dev, 0, sizeof(*dev));

	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = LIGHTS_DEVICE_API_VERSION_1_0;
	dev->common.module = (hw_module_t *)module;
	dev->common.close = (int (*)(hw_device_t *))close_lights;
	dev->set_light = set_light;

	*device = (hw_device_t *)dev;
	return 0;
}

__END_DECLS

static hw_module_methods_t lights_module_methods = {
	.open = open_lights,
};

/*
 * The lights Module
 */
hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = LIGHTS_HARDWARE_MODULE_ID,
	.name = "MI MAX 2 oxygen lights module",
	.author = "Xuefer",
	.methods = &lights_module_methods,
};

