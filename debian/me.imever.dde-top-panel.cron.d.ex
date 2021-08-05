#
# Regular cron jobs for the me.imever.dde-top-panel package
#
0 4	* * *	root	[ -x /usr/bin/me.imever.dde-top-panel_maintenance ] && /usr/bin/me.imever.dde-top-panel_maintenance
