#
_XDCBUILDCOUNT = 
ifneq (,$(findstring path,$(_USEXDCENV_)))
override XDCPATH = /Applications/ti/simplelink_cc13x0_sdk_4_20_00_05/source;/Applications/ti/simplelink_cc13x0_sdk_4_20_00_05/kernel/tirtos/packages
override XDCROOT = /Applications/ti/ccs1030/xdctools_3_62_00_08_core
override XDCBUILDCFG = ./config.bld
endif
ifneq (,$(findstring args,$(_USEXDCENV_)))
override XDCARGS = 
override XDCTARGETS = 
endif
#
ifeq (0,1)
PKGPATH = /Applications/ti/simplelink_cc13x0_sdk_4_20_00_05/source;/Applications/ti/simplelink_cc13x0_sdk_4_20_00_05/kernel/tirtos/packages;/Applications/ti/ccs1030/xdctools_3_62_00_08_core/packages;..
HOSTOS = MacOS
endif
