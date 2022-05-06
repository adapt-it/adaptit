ARG FROM
FROM ${FROM}

RUN if grep -qs eoan /etc/*release*; then \
      sed -i -re 's/([a-z]{2}.)?archive.ubuntu.com|security.ubuntu.com/old-releases.ubuntu.com/g' /etc/apt/sources.list; \
    fi

# Installs the `dpkg-buildpackage` command
RUN apt-get -y update && \
    DEBIAN_FRONTEND=noninteractive apt-get -y install \
    build-essential debhelper devscripts equivs \
    automake autoconf libtool gcc

# Copies your code file from your action repository to the filesystem path `/` of the container
COPY build.sh /build.sh

CMD ["/build.sh"]
