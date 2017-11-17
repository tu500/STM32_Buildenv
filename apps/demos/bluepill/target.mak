#DEMOS = blinky timer_blinky pwm_blinky pwm semihosting ssd1306
DEMOS = blinky enc28j60 timer_blinky semihosting ssd1306
SUBDIRS = $(DEMOS)
$(call include-subdirs)
