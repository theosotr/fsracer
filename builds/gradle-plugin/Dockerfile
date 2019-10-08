FROM ubuntu:18.04


# Install dependencies
ENV deps="sudo git vim wget curl strace gradle openjdk-11-jdk zip python3-pip git"
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

# Build gradle plugin
WORKDIR /root/plugin
RUN gradle build

ENV PLUGIN_JAR_DIR=/root/plugin/build/libs/
WORKDIR /root
