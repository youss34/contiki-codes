all: rpl_led rpl_led_root
APPS=servreg-hack
CONTIKI=/home/user/contiki-2.7

WITH_UIP6=1
UIP_CONF_IPV6=1
CFLAGS+= -DUIP_CONF_IPV6_RPL

include $(CONTIKI)/Makefile.include
