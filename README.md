# SO-ARTN RTS2 Branch
Official so-artn branch of rts2. Looking at the rts2 installed and running on bigartn, it was the branch "ngdome". I have created a new branch, this one, from ngdome. This will be our master branch from now on. 

## git remote show origin
```bash
* remote origin
  Fetch URL: https://github.com/RTS2/rts2
  Push  URL: https://github.com/RTS2/rts2
  HEAD branch: master
  Remote branches:
    kuiper61                   tracked
    lowell_updates             new (next fetch will store in remotes/origin)
    master                     tracked
    pkubanek-newconn           tracked
    pkubanek-newmodel          tracked
    pkubanek-python3           tracked
    refs/remotes/origin/lowell stale (use 'git remote prune' to remove)
    replaced_select_with_poll  tracked
    svn/ASUAV                  tracked
    svn/REL_0_5_0_exp          tracked
    svn/REL_0_7_3              tracked
    svn/REL_0_7_5              tracked
    svn/REL_0_8_0              tracked
    svn/REL_0_8_1              tracked
    svn/REL_0_8_1@7710         tracked
    svn/REL_0_9_0              tracked
    svn/REL_0_9_1              tracked
    svn/REL_0_9_2              tracked
    svn/REL_0_9_3              tracked
    svn/REL_0_9_4              tracked
    svn/cipek                  tracked
  Local branch configured for 'git pull':
    master merges with remote master
  Local ref configured for 'git push':
    master pushes to master (local out of date)
mtnops@bigartn:/home/scott/git-clones/rts2$ git branch
  binning
  css_fits
  master
* ngdome
  scott

```
RTS2 - Remote Telescope System, 2nd Version
===========================================

RTS2 is software for a fully autonomous astronomical observatory. It takes care
of details necessary for the observatory operation. It keeps care of closing it
when bad weather arrives, opening when weather permits operation. One of its
driving requirements was to provide a modular and flexible environment, able to
control a lot of (quickly changing) devices.

RTS2 is split into two main components - the RTS2 library, released under LGPL,
and RTS2 drivers, released under GPL. Pleasse see COPYING.LESSER and COPYING and 
header of source files for details.

RTS2 is not an interactive planetarium. RTS2 provides various interfaces which
third parties can use to access system functionalities. Probably the best is
JSON API. [Stellarium](http://stellarium.org) has plugin to interact with RTS2
telescopes.

For further details, please visit:

  [http://www.rts2.org](http://www.rts2.org)

For questions, comments, suggestions and problems, please send emails to petr
(at) rts2 [dot] org or to the RTS2 developer list rts-2-devel (at) lists
[dot] sourceforge [dot] net. Please be aware that support is first provided
to parties sharing development and implementation costs.

How to install RTS2
===================

The easiest way on Ubuntu/debian based distributions is to install RTS2 from packages.
The packages are available from [Launchpad](https://launchpad.net/~rts2/+archive/ubuntu/daily).

If you want to install from the source code, please see the [INSTALL](./INSTALL) file.
