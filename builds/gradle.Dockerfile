ARG IMAGE_NAME=ubuntu:18.04
FROM ${IMAGE_NAME}

USER root
# Install dependencies
ENV deps="sudo git vim wget curl strace gradle openjdk-8-jdk zip python3-pip git"
RUN apt update && apt install -y $deps

# Install Kotlin
RUN wget -O sdk.install.sh "https://get.sdkman.io" && bash sdk.install.sh
RUN bash -c "source ~/.sdkman/bin/sdkman-init.sh && sdk install kotlin"

# Install python packages
RUN pip3 install requests beautifulsoup4

# Install Android SDK.
ENV ANDROID_TOOLS=https://dl.google.com/android/repository/sdk-tools-linux-4333796.zip
RUN apt update && apt install -y android-sdk

# Accept licenses.
WORKDIR /root
RUN update-java-alternatives --set java-1.8.0-openjdk-amd64
RUN wget $ANDROID_TOOLS -O tools.zip && unzip tools.zip
RUN yes | /root/tools/bin/sdkmanager --licenses && \
  cp -r /root/licenses /usr/lib/android-sdk

# Copy necessary files
USER fsmove
WORKDIR $HOME

RUN mkdir plugin
ENV ANDROID_SDK_ROOT=/usr/lib/android-sdk
ENV ANDROID_HOME=/usr/lib/android-sdk

RUN sudo chown -R fsmove:fsmove ${ANDROID_SDK_ROOT}

COPY ./syscalls.txt $HOME/syscalls.txt
COPY ./gradle-plugin/build.gradle $HOME/plugin
COPY ./gradle-plugin/src $HOME/plugin/src
COPY ./gradle-filters.json $HOME/gradle-filters.json

# Build gradle plugin
WORKDIR $HOME/plugin
RUN gradle build

ENV PLUGIN_JAR_DIR=$HOME/plugin/build/libs/

COPY ./gradle-plugin/scripts $HOME/plugin/scripts
COPY ./tools/adapter.py /usr/local/bin/adapter.py

WORKDIR $HOME
