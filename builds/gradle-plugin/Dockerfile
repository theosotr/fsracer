FROM ubuntu:18.04


# Install dependencies
ENV deps="sudo git vim wget curl strace gradle openjdk-11-jdk zip"
RUN apt update && apt install -y $deps

# Install Kotlin
RUN wget -O sdk.install.sh "https://get.sdkman.io" && bash sdk.install.sh
RUN bash -c "source ~/.sdkman/bin/sdkman-init.sh && sdk install kotlin"


WORKDIR /root
