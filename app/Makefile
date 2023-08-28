# name of your application
APPLICATION = mqttsn_publisher

# If no BOARD is found in the environment, use this default:
BOARD ?= avr-rss2

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../RIOT-OS
#RIOTBASE ?= $(CURDIR)/../../../../../../RIOT-OS/

# Network stack to use - sim7020 or gnrc
NETSTACK ?= sim7020
CFLAGS += -DSIM7000G

# MQTT-SN publisher config
#
# Path to module for MQTT-SN publisher 
EXTERNAL_MODULE_DIRS += $(CURDIR)/..
INCLUDES += -I$(CURDIR)/..
USEMODULE += mqttsn_publisher
# Enable publisher thread
CFLAGS += -DMQTTSN_PUBLISHER_THREAD
# Autolauch at startup
CFLAGS += -DAUTO_INIT_MQTTSN
# Use DNS to lookup host names
#changed false to true
USE_DNS ?= true
# Use SNTP to syncrhonize clocks
CFLAGS += -DSNTP_SYNC
# MQTT-SN gateway
# lxc-ha IPv6 static ULA:
#CFLAGS += -DMQTTSN_GATEWAY_HOST=\"fd95:9bba:768f:0:216:3eff:fec6:99db\" 
#minime.sjodin.net:
#CFLAGS += -DMQTTSN_GATEWAY_HOST=\"::ffff:82ed:ca25\"
#lab-pc.ssvl.kth.se
CFLAGS += -DMQTTSN_GATEWAY_HOST=\"::ffff:c010:7de8\"
#CFLAGS += -DMQTTSN_GATEWAY_HOST=\"2001:6b0:32:13::232\"
#CFLAGS += -DMQTTSN_GATEWAY_HOST=\"lab-pc.ssvl.kth.se\"
CFLAFS += DMQTTSN_GATEWAY_PORT=10000

USEMODULE += emcute
CFLAGS += -DEMCUTE_ID=\"rpl-857b\"

# Use SNTP for global timestamps
USEMODULE += sntp
USEMODULE += gnrc_netif
USEMODULE += gnrc_netapi

ifeq ($(NETSTACK), sim7020)
  USEMODULE += async_at #added
  USEMODULE += sim7020_sock_udp
  ifeq ($(USE_DNS), true)
    USEMODULE += sim7020_sock_dns
  endif
  USEMODULE += auto_init_sim7020
  CFLAGS += -DCONFIG_AT_SEND_SKIP_ECHO=1 #added
else ifeq ($(NETSTACK), gnrc)
  ifeq ($(USE_DNS), true)
    USEMODULE += sock_dns
  endif
  # Include packages that pull up and auto-init the link layer.
  # NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
  USEMODULE += gnrc_netdev_default
  USEMODULE += auto_init_gnrc_netif
  # Activate ICMPv6 error messages
  USEMODULE += gnrc_icmpv6_error
  # Specify the mandatory networking modules for IPv6 and UDP
  USEMODULE += gnrc_ipv6_router_default
  USEMODULE += gnrc_udp
  # Add a routing protocol
  USEMODULE += gnrc_rpl
  USEMODULE += gnrc_rpl_srh
  USEMODULE += auto_init_gnrc_rpl
  # This application dumps received packets to STDIO using the pktdump module
  #USEMODULE += gnrc_pktdump
  # Additional networking modules that can be dropped if not needed
  USEMODULE += gnrc_icmpv6_echo
  # Autoinit RPL on single interface
  USEMODULE += gnrc_netif_single
endif

USEMODULE += core_mbox
# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += schedstatistics
ifeq ($(NETSTACK), gnrc)
USEMODULE += netstats_l2
USEMODULE += netstats_ipv6
USEMODULE += netstats_rpl
endif
USEMODULE += xtimer
USEMODULE += at24mac
#added
USEMODULE += hashes
USEMODULE += timex

ifeq ($(NETSTACK), sim7020)
CFLAGS += -DAT_PRINT_INCOMING=1 -DAT_PRINT_OUTGOING=1
endif

CFLAGS += -DDEBUG_ASSERT_VERBOSE

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

ifeq ($(NETSTACK), gnrc)
  # Uncomment the following 2 lines to specify static link lokal IPv6 address
  # this might be useful for testing, in cases where you cannot or do not want to
  # run a shell with ifconfig to get the real link lokal address.
  #IPV6_STATIC_LLADDR ?= '"fe80::cafe:cafe:cafe:1"'
  #CFLAGS += -DGNRC_IPV6_STATIC_LLADDR=$(IPV6_STATIC_LLADDR)

  # Uncomment this to join RPL DODAGs even if DIOs do not contain
  # DODAG Configuration Options (see the doc for more info)
  # CFLAGS += -DGNRC_RPL_DODAG_CONF_OPTIONAL_ON_JOIN

  # DNS with gnrc needs DNS resolver
  # changed DDNS to DNS
  ifeq ($(USE_DNS), true)
    CFLAGS += -DNS_RESOLVER=\"2001:4860:4860::8888\"
  endif
endif

# Override I2C defaults for lower speed
CFLAGS += -DI2C_NUMOF=1U -DI2C_BUS_SPEED=I2C_SPEED_NORMAL

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Specify custom dependencies for your application here ...
ifeq ($(NETSTACK), gnrc)
  APPDEPS += gnrc_rpl.o
endif

include $(RIOTBASE)/Makefile.include

# ... and define them here (after including Makefile.include,
# otherwise you modify the standard target):

#	./script.py

# Set a custom channel 
# For KTH Contiki gateway
#
DEFAULT_CHANNEL=25
DEFAULT_PAN_ID=0xFEED
# changed riotmake to riotbase
include $(RIOTBASE)/makefiles/default-radio-settings.inc.mk
#include $(RIOTMAKE)/default-radio-settings.inc.mk
