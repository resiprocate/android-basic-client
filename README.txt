
This is an Android library for using reSIProcate on Android.

You must have the Android SDK, the NDK and a recent version of Gradle.

1. Checkout the reSIProcate ndkports fork and this demo from git:

      git clone https://github.com/resiprocate/resiprocate.git
      git clone https://github.com/resiprocate/ndkports.git
      git clone https://github.com/resiprocate/android-basic-client.git

2. Put a reSIProcate tarball into /tmp

      cd resiprocate
      build/release-tarball.sh
      cp resiprocate-1.13.0~alpha1.tar.gz /tmp/reSIProcate-snapshot.tar.gz

3. Build AAR files for OpenSSL and reSIProcate in ndkports:

      cd ../ndkports
      git checkout pocock/resiprocate
      export PATH=/opt/gradle-7.3.3/bin/:$PATH
      export ANDROID_SDK_ROOT=~/Android/Sdk/
      gradle -PndkPath=/home/daniel/Android/Sdk/ndk/23.1.7779620 release -x test

4. Build android-basic-client

      cd ../android-basic-client
      export PATH=/opt/gradle-7.3.3/bin/:$PATH
      export ANDROID_SDK_ROOT=~/Android/Sdk/
      gradle -PndkPath=/home/daniel/Android/Sdk/ndk/23.1.7779620 assemble publishToMavenLocal -x test

Now you have a library that you can include in your app.

The instructions for your app show the next steps to build the app
and deploy it to a phone.

Notes:
* discovery of DNS servers from system properties net.dns1 and net.dns2
* if you have DNS problems, try sending to an IP address
* reSIProcate's logging messages are logged to the Android
  logging facility and they can be monitoring with `adb logcat`
* To create more API skeleton code, use javah, e.g:
  javah -o jni/foo.cpp -cp src org.resiprocate.android.basiccall.SipCall

Debugging tips
--------------

As this is a demo app, AndroidManifest.xml includes debuggable=true.
Remove that for production use.

For debugging the JNI / C++ code at runtime on the phone:

Make sure the dependencies in the AAR files are not stripped or
optimized.  At the time of writing this, ndkports build system
doesn't appear to be stripping the libraries.
ndkports/resiprocate/build.gradle.kts includes CXXFLAGS
for (de-)optimization.

In the android-demo-message build.gradle file, make sure the doNotStrip
option is not commented.

After building the APK, you may want to unpack it with the jar command
and verify that each shared object is unstripped.  E.g:

    mkdir foo
    cd foo
    jar xf ../build/outputs/apk/debug/android-demo-message-debug.apk
    file lib/*/*.so

The correct output looks like this:

    lib/arm64-v8a/libresip.so: ELF 64-bit LSB shared object, ARM aarch64, version 1 (SYSV), dynamically linked, with debug_info, not stripped

To use gdbserver on any arbitrary process in the phone, you need to
root your phone.

In this demo, we run the reSIProcate code as an Android Service.  Android
runs a Service in a separate process with a distinct PID.

In this example, the process 12790 is the Service:

134|hlte:/ # ps | grep basicmessage
u0_a85    12238 376   1023828 52544 sys_epoll_ b5904054 S org.resiprocate.android.basicmessage
u0_a85    12790 376   989340 45252 sys_epoll_ b5904054 S org.resiprocate.android.basicmessage:remote

Use a command like this to run gdbserver in the phone:

hlte:/ # gdbserver --attach :5045 `ps | grep basicmessage:remote \
              | tr -s ' ' | cut -f2 -d' '`

On the development workstation, set up port 5045:

    adb forward tcp:5045 tcp:5045

Now you can run the gdb distributed in the NDK:

    ${ANDROID_NDK_HOME}/prebuilt/linux-x86_64/bin/gdb

In the gdb session:

    (gdb)  target remote :5045
    Remote debugging using :5045



http://www.resiprocate.org

Copyright (C) 2013-2022 Daniel Pocock
https://danielpocock.com  <daniel@pocock.com>
https://softwarefreedom.institute

