#! /bin/sh

SOTABASE=/home/root

if [ -f ${SOTABASE}/base-version/update_info.json ]; then
	rm -f ${SOTABASE}/base-version/update_info.json
fi

sotaclient -i /etc/client_info.json -s 122.165.96.181

if [ -f ${SOTABASE}/base-version/update_info.json ]; then
	sotaupdater -i ${SOTABASE}/base-version/update_info.json -o /etc/client_info.json -f
fi

