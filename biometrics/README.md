# goodix fix on oreo #

1. Copy to a new folder in device tree.

2. Delete fingerprint wrapper from device tree (if it exists).

3. In device.mk replace :-

   PRODUCT_PACKAGES + = \
      android.hardware.biometrics.fingerprint@2.0-service

				 with

   PRODUCT_PACKAGES + = \
      android.hardware.biometrics.fingerprint@2.0-service-custom

4. Current implimentation supports both sensors fpc and goodix.

5. Do not use some notifying cancel hacks like
   https://github.com/AOSPA/android_frameworks_base/commit/acbb85687fc577ff74d0a1de93ccd63c2b05985a
   it is already implimented in Biometricservice and may cause issues.

6. SuperSu before SR5-superSU-v.2.82 seems to have an issue so please use latest SuperSu.
