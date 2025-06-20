# jo

![jo logo](tests/jo-logo.png)

This is `jo`, a small utility to create JSON objects

```bash
$ jo -p name=jo n=17 parser=false
{
    "name": "jo",
    "n": 17,
    "parser": false
}
```

or arrays

```bash
$ seq 1 10 | jo -a
[1,2,3,4,5,6,7,8,9,10]
```

It has a [manual](jo.md), and you can read [why I wrote jo](http://jpmens.net/2016/03/05/a-shell-command-to-create-json-jo/).

## Build from Release tarball

To build from [a release](https://github.com/jpmens/jo/releases) you will need a C compiler to install from a source tarball which you download from the [Releases page](https://github.com/jpmens/jo/releases).

```bash
tar xvzf jo-1.3.tar.gz
cd jo-1.3
autoreconf -i
./configure
make check
make install
```


## Build from Github

[![Build Status](https://api.travis-ci.com/jpmens/jo.svg?branch=master)](https://travis-ci.com/github/jpmens/jo)

To install from the repository, you will need a C compiler as well as a relatively recent version of _automake_ and _autoconf_.

```bash
git clone https://github.com/jpmens/jo.git
cd jo
autoreconf -i
./configure
make check
make install
```

## Install

### Homebrew

```bash
brew install jo
```

### MacPorts

```bash
sudo port install jo
```

### Ubuntu

```
apt-get install jo
```

### Gentoo

```
emerge jo
```

### Fedora

```
dnf install jo
```

### Snap

Thanks to [Roger Light](https://twitter.com/ralight/status/1166023769623867398), _jo_ is available as a [snap package](https://snapcraft.io/jo). Use `snap install jo` from a Linux distro that supports snaps.

### Windows
```cmd
scoop install jo
```

### Windows WSL2

As shown in [#175](https://github.com/jpmens/jo/issues/175) when using _git_ on Windows WSL2 it should be necessary to disable automatic CRLF conversion in _git_ or the tests will fail:

```cmd
git config --local core.autocrlf false
```

### AIX

_jo_ builds and passes all tests on AIX 7.1 using the _autoconf_, _automake_, _gcc_, and _pkg-config_ RPMs from IBM's [AIX Toolbox for Open Source Software](https://www.ibm.com/support/pages/node/883796).  The _xlclang_ compiler from IBM's xlC/C++ suite for AIX will also build _jo_.

## Others

* [voidlinux](https://github.com/voidlinux/void-packages/tree/master/srcpkgs/jo)
* [ArchLinux](https://archlinux.org/packages/extra/x86_64/jo/)
* [OpenBSD](http://openports.se/textproc/jo)
* [FreeBSD](https://www.freshports.org/textproc/jo)
* [Guix](https://packages.guix.gnu.org/packages/jo/)
* [pkgsrc](http://pkgsrc.se/textproc/jo)
* [repology.org](https://repology.org/metapackage/jo/versions)
* [Docker](https://hub.docker.com/repository/docker/jpmens/jo)

## See also

* [gjo](https://github.com/skanehira/gjo)
* [rjo](https://github.com/dskkato/rjo)
* [jjo](https://github.com/memoryhole/jjo)
* [jf](https://github.com/sayanarijit/jf)
* [gojo](https://github.com/itchyny/gojo)

## Credits

* `json.[ch]` by 2011 Joseph A. Adams (joeyadams3.14159[at]gmail.com).
