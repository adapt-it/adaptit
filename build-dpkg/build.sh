#!/bin/sh
set -e

# Set the install command to be used by mk-build-deps (use --yes for non-interactive)
install_tool="apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends --yes"

# Install build dependencies automatically
mk-build-deps --install --tool="${install_tool}" debian/control

if [ "${LOCAL}" ]; then
  echo "Adding +${LOCAL} to package revision"
  debchange --local=+${LOCAL} "Build for ${LOCAL}"
  # Remove unwanted trailing 1 from distribution suffix
  sed -i -e "1s/+${LOCAL}1/+${LOCAL}/" debian/changelog
fi

# Build the package
dpkg-buildpackage $@

## Move the built artifacts into the Docker mounted workspace
mkdir -p artifacts
mv ../*.deb ../*.changes ../*.dsc ../*.tar.gz ./artifacts
