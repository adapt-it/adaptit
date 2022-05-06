# Notes on the Github Actions workflow to build deb packages

The Github Actions workflow builds deb packages for Ubuntu trusty,
xenial, bionic, eoan, focal, and impish. Also builds Deb packages for
Deiban sid.

The workflow is defined in .github/workflows/build-dpkg.yml.

The distro releases to build are defined as a matrix in the workflow.
The matrix is generated dynamically by the scripts/get-ci-matrix.py
script. It reads from the `RELEASES.yaml` file to get the matrix data.
For manually dispatched trigger, the user can specify a subset of the
releases to build and the script will generate a subset matrix with only
those releases.

The package build is run using a docker image that is defined in
`build-dpkg/`. The Dockerfile uses a build arg for the FROM line. For
each distro release version, the docker image for that release is built
and then used for the actual package build (e.g. dpkg-buildpackage).

The upload of additional release files is handled by a final job. This
is because with the same release file listed in the multiple jobs,
occasionally gives a already_exists error.

## Other notes:

- Add automake to build deps.
- For sid, the compat version we are using is too old so bump it from
  5 to 7.
- Call autogen.sh as step to avoid this error (due to timestamps not
  saved by git);
    make[1]: Entering directory '/github/workspace/bin/linux/Unicode'
    CDPATH="${ZSH_VERSION+.}:" && cd .. && /bin/bash /github/workspace/bin/linux/missing aclocal-1.15 -I m4
    /github/workspace/bin/linux/missing: line 81: aclocal-1.15: command not found
- The build-dpkg image is invoked using a LOCAL environment variable
  that uses debchange to append +LOCAL to each built file.
- The adaptit folder is checked out to a relative path `adaptit`. This
  is necessary because the built files are stored above that directory.
  If a relative path is not used, then the files are generated outside
  the docker volume mount and the following error happens:
    dh_builddeb: mv debian/.debhelper/scratch-space/build-adaptit/adaptit-dbgsym_6.10.6-1\+bionic_amd64.deb ../adaptit-dbgsym_6.10.6-1\+bionic_amd64.ddeb: Invalid cross-device link
            Renaming adaptit-dbgsym_6.10.6-1+bionic_amd64.deb to adaptit-dbgsym_6.10.6-1+bionic_amd64.ddeb
- For trusty and xenial, there are some packages that aren't pulled in
  by buildessential such as gcc, automake, and libtool. So add those
  explicitly. For eoan, we need to patch the sources.list file to get the
  old release packages.

## Links:

- Debian maintainers guide: https://www.debian.org/doc/manuals/maint-guide/
- Github Actions documentation: https://docs.github.com/en/actions
- Workflow file syntax reference: https://docs.github.com/en/actions/using-workflows/workflow-syntax-for-github-actions

