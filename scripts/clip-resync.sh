#!/bin/bash
# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright © 2007-2018 ANSSI. All Rights Reserved.

###################################################################################
#
# EADS DCS
#
# Resync disk
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version
# 2 as published by the Free Software Foundation.
#
####################################################################################

#Tools
LS="/bin/ls"
DD="/bin/dd"
LOGGER="/usr/bin/logger"
XDIALOG="/usr/local/bin/Xdialog"
SFDISK="/sbin/sfdisk"
AWK="/usr/bin/awk"
SED="/bin/sed"
MDADM="/sbin/mdadm"
GREP="/bin/grep"
HEAD="/usr/bin/head"

#Error messages
ERROR_BAD_INDEVICE="Impossible de trouver un disque source valide. Essayez de rebooter la machine sans le nouveau disque, inserez-le avant de relancer la manipulation sur sdb"
ERROR_BAD_OUTDEVICE="Le disque destination est invalide. Essayez de rebooter la machine sans le nouveau disque, inserez-le avant de relancer la manipulation sur sdb"

####################################
############ Util code #############
####################################

function error()
{
    ${LOGGER} -i -s -p kern.crit -t "ERR RESYNC" -- "${1}"
    
    # Get the authority file
    AUTHORITY=$(${LS} -1t /tmp/.Xauth* | ${HEAD} -1)
    
    XDIALOG_MESSAGE="Erreur : ${1}"
    XDIALOG_TITLE="Re-synchronisation des disques durs"
   
    DISPLAY=:0.0 XAUTHORITY=${AUTHORITY} ${XDIALOG} --title "${XDIALOG_TITLE}" --msgbox "${XDIALOG_MESSAGE}" 6 60

    exit -1
}

function debug()
{
    ${LOGGER} -i -s -p kern.info -t "RESYNC" -- "${1}"
}

####################################
############ MAIN CODE #############
####################################

OUTDEVICE="${1}"
INDEVICE="sda"

# gets partition table from a valid device
echo "Recherche de la table de partition de ${INDEVICE}"
${SFDISK} -d /dev/${INDEVICE} > /tmp/partable
if [ $? -ne 0 ] ; then
	INDEVICE="sdb"
	echo "Recherche de la table de partition de ${INDEVICE}"
	${SFDISK} -d /dev/${INDEVICE} > /tmp/partable
	if [ $? -ne 0 ] ; then
		# this may happen if two valid device failed without admin rebooted
		# so a reboot is requested
		error "${ERROR_BAD_INDEVICE}"
	fi
fi

# puts new partition table and boot sector
echo "Partition de ${OUTDEVICE}"
${SFDISK} /dev/${OUTDEVICE} < /tmp/partable
if [ $? -ne 0 ] ; then
	error "${ERROR_BAD_OUTDEVICE}"
fi
${DD} if=/dev/${INDEVICE} of=/dev/${OUTDEVICE} bs=512 count=1

# adds new partitions to virtual arrays
echo "Synchronization de ${OUTDEVICE}"
ARRAYLIST=`${AWK} '/md/ {print $1}' < /proc/mdstat`
for array in $ARRAYLIST; do
	outpart=`echo $array|${SED} -e s/md/${OUTDEVICE}/`
	${MDADM} /dev/${array} --add /dev/${outpart}
done

# removes faulty partitions
echo "Nettoyage des tables virtuelles"
FAULTYLIST=`${GREP} '(F)' /proc/mdstat | ${SED} -e 's/ /\n/g' | ${GREP} '(F)' | ${SED} -e 's/\[.*\](F)//'`
for faultypart in ${FAULTYLIST}; do
	array=`echo $faultypart|${SED} -e s/sd./md/`
	${MDADM} /dev/${array} --remove /dev/${faultypart}
done

exit 0
