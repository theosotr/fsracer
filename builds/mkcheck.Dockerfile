# vim:set ft=dockerfile:
FROM schaliasos/sbuild

ENV dev="git make vim wget cmake clang libboost-all-dev"

USER root
WORKDIR root

# INSTALL PACKAGES
RUN apt -yqq update && apt -yqq upgrade && apt install -yqq $dev

# INSTALL mkcheck
RUN git clone https://github.com/nandor/mkcheck
WORKDIR mkcheck
COPY ./patches/mkcheck/syscall.cpp mkcheck/syscall.cpp
RUN mkdir Release && cd Release && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ && \
    make
RUN install ./Release/mkcheck /usr/local/bin/
COPY ./scripts/mkcheck/fuzz_test /usr/local/bin


WORKDIR /root

COPY ./config/mkcheck/sbuildrc /root/.sbuildrc
COPY ./config/mkcheck/sbuildrc /home/builder/.sbuildrc
COPY ./config/mkcheck/fstab /etc/schroot/sbuild/fstab
COPY ./scripts/mkcheck/entrypoint.sh /usr/local/bin/entrypoint.sh
COPY ./scripts/mkcheck/analyzer /usr/local/bin/analyzer

# DIRECTORY TO SAVE STATS
RUN mkdir -p /var/log/sbuild/stats
RUN chown -R builder /var/log/sbuild/stats
run mkdir -p /results
RUN chown -R builder /results
RUN chmod o+w /results

USER builder
WORKDIR /home/builder

ENTRYPOINT ["entrypoint.sh"]
