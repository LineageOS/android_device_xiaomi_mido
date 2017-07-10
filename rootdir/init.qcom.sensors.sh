#!/system/bin/sh
# Copyright (c) 2015, The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of The Linux Foundation nor
#       the names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
# Function to start sensors for SSC enabled platforms
#
start_sensors()
{
    if [ -c /dev/msm_dsps -o -c /dev/sensors ]; then
        if [ ! -d /persist/sensors ]
        then
            mkdir /persist/sensors
        fi
        chmod 0775 /persist/sensors
        chown sensors.sensors /persist/sensors

        if [ ! -f /persist/sensors/sensors_settings ]; then
            echo 1 > /persist/sensors/sensors_settings
        fi
        VALUE=`cat /persist/sensors/sensors_settings` 2> /dev/null
        if [ "$VALUE" != "1" -a "$VALUE" != "0" ]; then
            echo 1 > /persist/sensors/sensors_settings
        fi
        chmod 0664 /persist/sensors/sensors_settings
        chown system.root /persist/sensors/sensors_settings

        chcon -R u:object_r:sensors_persist_file:s0 /persist/sensors

        mkdir -p /data/misc/sensors
        chmod 0775 /data/misc/sensors

        chown system:system /persist/PRSensorData.txt
        chmod 0600 /persist/PRSensorData.txt
        chcon u:object_r:sensors_persist_file:s0 /persist/PRSensorData.txt

        chown system:system /persist/PSensor3cm_ct.txt
        chcon u:object_r:sensors_persist_file:s0 /persist/PSensor3cm_ct.txt
        chmod 0600 /persist/PSensor3cm_ct.txt

        start sensors
    fi
}

start_sensors
