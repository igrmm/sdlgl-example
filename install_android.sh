#!/bin/sh

SDL_COMMIT=tags/release-2.28.3
ANDROID_PROJECT=com.igrmm.sdlgl
APP_NAME=SDLGL

mkdir -p build

if [ ! -d "build/android" ]; then
    cd build
    mkdir -p android && cd android
    git clone https://github.com/libsdl-org/SDL/
    cd SDL && git checkout $SDL_COMMIT && cd build-scripts
    ./androidbuild.sh $ANDROID_PROJECT /dev/null
    cd ../ && mv build/$ANDROID_PROJECT ../ && cd ../
    cd $ANDROID_PROJECT/app/jni/

    #scaping $ and /
    sed -i "s/LOCAL_SRC_FILES.*/LOCAL_SRC_FILES := \$(wildcard \$(LOCAL_PATH)\/..\/..\/..\/..\/..\/..\/src\/*.c)/" src/Android.mk
    sed -i "s/-lGLESv2/-lGLESv3/" src/Android.mk

    cd ../
    sed -i "s/Game/$APP_NAME/" src/main/res/values/strings.xml

    sed -i "/<activity android:name/a \            android:screenOrientation=\"landscape\"" src/main/AndroidManifest.xml

    mkdir src/main/res/values-v27/
    echo '<?xml version="1.0" encoding="utf-8"?>
    <resources>
    <style name="Theme">
    <item name="android:windowLayoutInDisplayCutoutMode">
    shortEdges
    </item>
    </style>
    </resources>' > src/main/res/values-v27/styles.xml
    sed -i "s/android:theme=.*/android:theme=\"@style\/Theme\"/" src/main/AndroidManifest.xml

    cd ../../../../
    ln -s $(pwd)/assets build/android/$ANDROID_PROJECT/app/src/main/
fi
cd build/android/$ANDROID_PROJECT/
./gradlew installDebug
cd ../../../
