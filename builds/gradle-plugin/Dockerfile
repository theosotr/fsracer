FROM ubuntu:18.04


# Install dependencies
ENV deps="sudo git vim wget curl strace gradle openjdk-8-jdk zip python3-pip git"
RUN apt update && apt install -y $deps

# Install Kotlin
RUN wget -O sdk.install.sh "https://get.sdkman.io" && bash sdk.install.sh
RUN bash -c "source ~/.sdkman/bin/sdkman-init.sh && sdk install kotlin"

# Install python packages
RUN pip3 install requests beautifulsoup4

# Copy necessary files
RUN mkdir /root/plugin

WORKDIR /root
COPY ./build.gradle /root/plugin
COPY ./src /root/plugin/src
COPY ./scripts /root/plugin/scripts
COPY ./syscalls.txt /root/syscalls.txt

# Install Android SDK and accept licenses.
ENV ANDROID_TOOLS=https://dl.google.com/android/repository/sdk-tools-linux-4333796.zip
RUN apt update && apt install -y android-sdk
ENV ANDROID_SDK_ROOT=/usr/lib/android-sdk
ENV ANDROID_HOME=/usr/lib/android-sdk
RUN update-java-alternatives --set java-1.8.0-openjdk-amd64
RUN wget $ANDROID_TOOLS -O tools.zip && unzip tools.zip
RUN yes | /root/tools/bin/sdkmanager --licenses && \
  cp -r /root/licenses $ANDROID_SDK_ROOT

# Build gradle plugin
WORKDIR /root/plugin
RUN gradle build


ENV PLUGIN_JAR_DIR=/root/plugin/build/libs/
WORKDIR /root
