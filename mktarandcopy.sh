rm nl80211test-1.0.0.tar.gz
tar -cvzf nl80211test-1.0.0.tar.gz *

echo
echo
echo "*** COMPLETE, COPYING.... ***"
#  cp nl80211test-1.0.0.tar.gz /home/office/yocto/poky/meta-shadowx/recipes-shadowx/nl80211/nl80211test-1.0.0/
# cp nl80211test-1.0.0.tar.gz /home/srtg/yocto/poky/meta-gumstix-extras/recipes-shadowx/nl80211/nl80211test-1.0.0
cp nl80211test-1.0.0.tar.gz /home/srtg/yocto/poky/meta-shadowx/recipes-shadowx/nl80211/nl80211test-1.0.0/
echo "Local copy:"
ls -l nl80211test-1.0.0.tar.gz

echo "bitbake's copy:"
#  ls -l /home/office/yocto/poky/meta-shadowx/recipes-shadowx/nl80211/nl80211test-1.0.0/
# ls -l /home/srtg/yocto/poky/meta-gumstix-extras/recipes-shadowx/nl80211/nl80211test-1.0.0/
ls -l /home/srtg/yocto/poky/meta-shadowx/recipes-shadowx/nl80211/nl80211test-1.0.0/

