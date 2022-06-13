import os
from productInfo import get_SKUs_of_family, get_HW_VERS_of_SKU, SKUs_to_MTUTypes, MTUTypes_to_Confs
from xmlBuilder.xml_file import XMLFile
from xmlBuilder.xmlChecker import check_xml_for_errors


# This should be incremented every time a new version is released (into TeamCenter)
# Also, this must be reset to "A" whenever the major/minor version is incremented
_DocCtrlRev = "A"


def gen_xml_file(sku, major='00', minor='00', build='0000', crc='0000', desired_doc_control_rev='', HW_VER=0, output_dir=None):
    """

    :param sku:
    :param str major: nn
    :param str minor: nn
    :param str build: nnnn
    :param str crc: nnnn in Hex
    :param desired_doc_control_rev:
    :param HW_REV:
    :return:
    """
    MTUType = SKUs_to_MTUTypes[sku]

    internalAntenna = sku[-2:] != "-A"

    bldLSB = build[2:4]
    bldMSB = build[0:2]

    crcLsb = crc[2:4]
    crcMsb = crc[0:2]

    versionString = major + "." + minor


    try:
        mtu_conf = MTUTypes_to_Confs[MTUType]
        partNumber = mtu_conf.partNumber[HW_VER]
        portType = mtu_conf.portType
        meterType = mtu_conf.meterType
        mtuRange = mtu_conf.mtuRange
        hasRDD = mtu_conf.hasRDD
        antennaType = mtu_conf.antennaType
        isRemoteGas = mtu_conf.isRemoteGas
        formCReed = mtu_conf.formCReed
        flowIsCW = mtu_conf.flowIsCW
        FW_FAMILY = mtu_conf.FW_FAMILY

    except KeyError:
        raise TypeError("Unsupported MTUType.")

    if not internalAntenna:
        partNumber = partNumber + "-A"
        antennaType = "0"

    if hasRDD:
        xmlFilename = "{}__{}__V{}__{}__{}__{}+{}__{}_RANGE_HW_VER_{}".format(sku, partNumber, versionString, MTUType, portType,
                                                                            meterType, "RDD", mtuRange, HW_VER)
    else:
        xmlFilename = "{}__{}__V{}__{}__{}__{}__{}_RANGE_HW_VER_{}".format(sku, partNumber, versionString, MTUType, portType,
                                                                         meterType, mtuRange, HW_VER)

    docCtrlID = "{}-{}-MAP-V{}".format(sku, partNumber, versionString)

    if (meterType == "GAS_REMOTE") or (meterType == "GAS_DIRECT") or (meterType == "REMOTE_GAS_PULSE"):
        P1_MODE2 = 0

        if isRemoteGas:
            P1_MODE2 |= (1 << 4)

        if formCReed:
            P1_MODE2 |= (1 << 5)

        if flowIsCW:
            P1_MODE2 |= (1 << 6)

        P1_MODE2 = str(P1_MODE2)

    if portType == "2PORT":
        numTxBetweenRx = "56"
    else:
        numTxBetweenRx = "28"

    catalogNum = "{}[-XXX]".format(sku)

    partNumber = "20" + partNumber  # gives full year (e.g., 2017 instead of 17)

    gen_xml = XMLFile(DocControlRev=desired_doc_control_rev, DocControlID=docCtrlID,
                      PartNumber=partNumber, FirmwareVersion=versionString, CatalogNumber=catalogNum)

    gen_xml.add_entry(PropName="MTU Type", ShortName="MTU_TYPE", Addr="0", Mode="ca", Value=str(MTUType),
                      Size="1", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Time Sync Hour", ShortName="TS_HOUR", Addr="1", Mode="ca", Value="9", Size="1",
                      internalOnly=False, Description="time sync hour    Time of day to listen for Time Sync")

    gen_xml.add_entry(PropName="Time Sync Minute", ShortName="TS_MIN", Addr="2", Mode="ca", Value="38", Size="1",
                      internalOnly=False, Description="time sync minute    Time of day to listen for Time Sync")

    gen_xml.add_entry(PropName="Time Sync Second", ShortName="TS_SEC", Addr="3", Mode="ca", Value="30", Size="1",
                      internalOnly=False, Description="time sync second    Time of day to listen for Time Sync")

    gen_xml.add_entry(PropName="Time Sync Request Timeout", ShortName="TimeSyncReqTimeout", Addr="4", Mode="ca",
                      Value="96", Size="1", internalOnly=False,
                      Description="Number of hours without a Time Sync needed to raise an alarm.")
    if FW_FAMILY is not None:
        gen_xml.add_entry(PropName="MTU Firmware Family ID", ShortName="MTU_FAMILY_TYPE", Addr="5", Mode="ca",
                          Value=FW_FAMILY, Size="1", internalOnly=False,
                          Description="MTU Firmware Family. All MTU types with a common firmware family ID can accept the same DFW image.")

    gen_xml.add_entry(PropName="MTU ID Byte 0", ShortName="MTU_ID", Addr="6", Mode="no", Value="0", Size="4",
                      internalOnly=True, Description="")
    gen_xml.add_entry(PropName="MTU ID Byte 1", ShortName="MTU_ID", Addr="7", Mode="no", Value="0", Size="4",
                      internalOnly=True, Description="")
    gen_xml.add_entry(PropName="MTU ID Byte 2", ShortName="MTU_ID", Addr="8", Mode="no", Value="0", Size="4",
                      internalOnly=True, Description="")
    gen_xml.add_entry(PropName="MTU ID Byte 3", ShortName="MTU_ID", Addr="9", Mode="no", Value="0", Size="4",
                      internalOnly=True, Description="")

    gen_xml.add_entry(PropName="One way TX Byte 0", ShortName="ONE_WAY_TX", Addr="10", Mode="no", Value="204",
                      Size="4", internalOnly=True, Description="1 way rf com freq in Hz")
    gen_xml.add_entry(PropName="One way TX Byte 1", ShortName="ONE_WAY_TX", Addr="11", Mode="no", Value="61",
                      Size="4", internalOnly=True, Description="1 way rf com freq in Hz")
    gen_xml.add_entry(PropName="One way TX Byte 2", ShortName="ONE_WAY_TX", Addr="12", Mode="no", Value="218",
                      Size="4", internalOnly=True, Description="1 way rf com freq in Hz")
    gen_xml.add_entry(PropName="One way TX Byte 3", ShortName="ONE_WAY_TX", Addr="13", Mode="no", Value="27",
                      Size="4", internalOnly=True, Description="1 way rf com freq in Hz")

    gen_xml.add_entry(PropName="Two way TX Byte 0", ShortName="TWO_WAY_TX", Addr="14", Mode="no", Value="140",
                      Size="4", internalOnly=True, Description="2 way rf com freq in Hz")
    gen_xml.add_entry(PropName="Two way TX Byte 1", ShortName="TWO_WAY_TX", Addr="15", Mode="no", Value="242",
                      Size="4", internalOnly=True, Description="2 way rf com freq in Hz")
    gen_xml.add_entry(PropName="Two way TX Byte 2", ShortName="TWO_WAY_TX", Addr="16", Mode="no", Value="141",
                      Size="4", internalOnly=True, Description="2 way rf com freq in Hz")
    gen_xml.add_entry(PropName="Two way TX Byte 3", ShortName="TWO_WAY_TX", Addr="17", Mode="no", Value="27",
                      Size="4", internalOnly=True, Description="2 way rf com freq in Hz")

    gen_xml.add_entry(PropName="Two way RX Byte 0", ShortName="TWO_WAY_RX", Addr="18", Mode="no", Value="140",
                      Size="4", internalOnly=True, Description="2 way rf com freq in Hz")
    gen_xml.add_entry(PropName="Two way RX Byte 1", ShortName="TWO_WAY_RX", Addr="19", Mode="no", Value="242",
                      Size="4", internalOnly=True, Description="2 way rf com freq in Hz")
    gen_xml.add_entry(PropName="Two way RX Byte 2", ShortName="TWO_WAY_RX", Addr="20", Mode="no", Value="141",
                      Size="4", internalOnly=True, Description="2 way rf com freq in Hz")
    gen_xml.add_entry(PropName="Two way RX Byte 3", ShortName="TWO_WAY_RX", Addr="21", Mode="no", Value="27",
                      Size="4", internalOnly=True, Description="2 way rf com freq in Hz")

    gen_xml.add_entry(PropName="System Flags", ShortName="SYSTEM_F", Addr="22", Mode="ca", Value="1", Size="1",
                      internalOnly=False,
                      Description="system flags  bit0: set when unit in ship mode, turns most functions off except coil bit1: set to DISABLE 2 way tx / afc correction  bit2: set to echo valid rx packets on coil interface, normally for testing only  bit3:   set to enable 60 sec charge on caps")

    gen_xml.add_entry(PropName="Number of High Priority Message Transmits", ShortName="NumHighPriDataTransmissions", Addr="23",
                      Mode="ca", Value="3", Size="1", internalOnly=False,
                      Description="Number of times a high priority message is sent")

    gen_xml.add_entry(PropName="Number of Tx between Rx", ShortName="num_tx_between_rx", Addr="24", Mode="ca",
                      Value=numTxBetweenRx, Size="1", internalOnly=False,
                      Description="Number of tx between rx; rx will turn on 60 sec after a tx. set to 0 - no rx, 1 - rx after every tx, 2 - rx every other tx, 3 - rx every 3rd tx ...")

    gen_xml.add_entry(PropName="Number of Readings before TX (P1 and P2)", ShortName="MSG_OVERLAP_CNT",
                      Addr="25", Mode="ca", Value="6", Size="1", internalOnly=False,
                      Description="number of new deltas before transmitting 1-12")

    gen_xml.add_entry(PropName="Port Read Interval Byte 0", ShortName="READ_INTERVAL", Addr="26", Mode="ca",
                      Value="60", Size="2", internalOnly=False,
                      Description="read interval (range 5-65535), applies to P1 and P2")
    gen_xml.add_entry(PropName="Port Read Interval Byte 1", ShortName="READ_INTERVAL", Addr="27", Mode="ca",
                      Value="0", Size="2", internalOnly=False,
                      Description="read interval (range 5-65535), applies to P1 and P2")

    if meterType == "PULSE":
        # Pulse on port 1
        gen_xml.add_entry(PropName="P1 Pulse Meter Type (LSB)", ShortName="P1_METER_TYPE", Addr="28", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P1 Pulse Meter Type (MSB)", ShortName="P1_METER_TYPE", Addr="29", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P1 Pulse Ratio (LSB)", ShortName="P1_PHF", Addr="30", Mode="ca", Value="1",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")
        gen_xml.add_entry(PropName="P1 Pulse Ratio (MSB)", ShortName="P1_PHF", Addr="31", Mode="ca", Value="0",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")

        gen_xml.add_entry(PropName="P1 Pulse High Time", ShortName="P1_PULSE_HIGH_TIME", Addr="32", Mode="ca",
                          Value="10", Size="1", internalOnly=False,
                          Description="setting for pulse trail debounce 0-127 (*8192us)")

        gen_xml.add_entry(PropName="P1 Pulse Low Time", ShortName="P1_PULSE_LOW_TIME", Addr="33", Mode="ca",
                          Value="15", Size="1", internalOnly=False,
                          Description="setting for pulse debounce routine 0-15 (*1024us)")

        gen_xml.add_entry(PropName="P1 Port Config", ShortName="P1_PORT_CONFIG", Addr="34", Mode="ca",
                          Value="212", Size="1", internalOnly=False, Description="Port1 attached device and settings.")

        gen_xml.add_entry(PropName="P1 Meter ID Byte 0", ShortName="P1_METER_ID", Addr="36", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 1", ShortName="P1_METER_ID", Addr="37", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 2", ShortName="P1_METER_ID", Addr="38", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 3", ShortName="P1_METER_ID", Addr="39", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 4", ShortName="P1_METER_ID", Addr="40", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 5", ShortName="P1_METER_ID", Addr="41", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")

    elif meterType == "ENCODER":
        # Encoder on port 1
        gen_xml.add_entry(PropName="P1 Encoder Meter Type (LSB)", ShortName="P1_METER_TYPE", Addr="28",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P1 Encoder Meter Type (MSB)", ShortName="P1_METER_TYPE", Addr="29",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P1 Encoder Type", ShortName="P1_ENCODER_TYPE", Addr="30", Mode="ca",
                          Value="1", Size="1", internalOnly=False, Description="Port 1 Encoder Type.")

        if HW_VER == 0:
            gen_xml.add_entry(PropName="P1 Port Init Options", ShortName="P1_portInitOptions", Addr="31", Mode="ca", Value="5", Size="1", internalOnly=False, Description="Port 1 Initialization Options")
        else:
            gen_xml.add_entry(PropName="P1 Port Init Options", ShortName="P1_portInitOptions", Addr="31", Mode="ca", Value="11", Size="1", internalOnly=False, Description="Port 1 Initialization Options")

        gen_xml.add_entry(PropName="P1 Encoder Number of Digits", ShortName="P1_ENCODER_NUM_DIGITS", Addr="32",
                          Mode="ca", Value="8", Size="1", internalOnly=False,
                          Description="Port 1 Encoder Number of Digits.")

        gen_xml.add_entry(PropName="P1 Port Config", ShortName="P1_PORT_CONFIG", Addr="34", Mode="ca",
                          Value="164", Size="1", internalOnly=False, Description="Port1 attached device and settings.")

        gen_xml.add_entry(PropName="P1 Meter ID Byte 0", ShortName="P1_METER_ID", Addr="36", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 1", ShortName="P1_METER_ID", Addr="37", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 2", ShortName="P1_METER_ID", Addr="38", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 3", ShortName="P1_METER_ID", Addr="39", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 4", ShortName="P1_METER_ID", Addr="40", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 5", ShortName="P1_METER_ID", Addr="41", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")

    elif meterType == "EXT_ALARMS":
        # AdvAlarms/Kamstrup/Diehl on port 1
        gen_xml.add_entry(PropName="P1 E-ALM Meter Type (LSB)", ShortName="P1_METER_TYPE", Addr="28", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P1 E-ALM Meter Type (MSB)", ShortName="P1_METER_TYPE", Addr="29", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P1 E-ALM Type", ShortName="P1_ENCODER_TYPE", Addr="30", Mode="ca", Value="1",
                          Size="1", internalOnly=False, Description="Port 1 E-ALM Type.")

        if HW_VER == 0:
            gen_xml.add_entry(PropName="P1 Port Init Options", ShortName="P1_portInitOptions", Addr="31", Mode="ca", Value="5", Size="1", internalOnly=False, Description="Port 1 Initialization Options")
        else:
            gen_xml.add_entry(PropName="P1 Port Init Options", ShortName="P1_portInitOptions", Addr="31", Mode="ca", Value="11", Size="1", internalOnly=False, Description="Port 1 Initialization Options")

        gen_xml.add_entry(PropName="P1 E-ALM Number of Digits", ShortName="P1_ENCODER_NUM_DIGITS", Addr="32",
                          Mode="ca", Value="8", Size="1", internalOnly=False,
                          Description="Port 1 E-ALM Number of Digits.")

        gen_xml.add_entry(PropName="P1 Port Config", ShortName="P1_PORT_CONFIG", Addr="34", Mode="ca",
                          Value="164", Size="1", internalOnly=False, Description="Port1 attached device and settings.")

        gen_xml.add_entry(PropName="P1 Meter ID Byte 0", ShortName="P1_METER_ID", Addr="36", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 1", ShortName="P1_METER_ID", Addr="37", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 2", ShortName="P1_METER_ID", Addr="38", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 3", ShortName="P1_METER_ID", Addr="39", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 4", ShortName="P1_METER_ID", Addr="40", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 5", ShortName="P1_METER_ID", Addr="41", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")

    elif meterType == "GAS_REMOTE":
        # Remote Mount Gas on port 1
        gen_xml.add_entry(PropName="P1 Remote Mount Gas Meter Type (LSB)", ShortName="P1_METER_TYPE", Addr="28",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P1 Remote Mount Gas Meter Type (MSB)", ShortName="P1_METER_TYPE", Addr="29",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P1 Pulse Ratio (LSB)", ShortName="P1_PHF", Addr="30", Mode="ca", Value="1",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")
        gen_xml.add_entry(PropName="P1 Pulse Ratio (MSB)", ShortName="P1_PHF", Addr="31", Mode="ca", Value="0",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")

        gen_xml.add_entry(PropName="P1 Pulse High Time", ShortName="P1_PULSE_HIGH_TIME", Addr="32", Mode="ca",
                          Value="10", Size="1", internalOnly=False,
                          Description="setting for pulse trail debounce 0-127 (*8192us)")

        gen_xml.add_entry(PropName="P1 Pulse Low Time", ShortName="P1_PULSE_LOW_TIME", Addr="33", Mode="ca",
                          Value="15", Size="1", internalOnly=False,
                          Description="setting for pulse debounce routine 0-15 (*1024us)")

        gen_xml.add_entry(PropName="P1 Port Config", ShortName="P1_PORT_CONFIG", Addr="34", Mode="ca",
                          Value="180", Size="1", internalOnly=False, Description="Port1 attached device and settings.")

        gen_xml.add_entry(PropName="P1 MODE2 Config", ShortName="P1_MODE2_FLAGS", Addr="35", Mode="ca",
                          Value=P1_MODE2, Size="1", internalOnly=False, Description="Port1 Gas Flags.")

        gen_xml.add_entry(PropName="P1 Meter ID Byte 0", ShortName="P1_METER_ID", Addr="36", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 1", ShortName="P1_METER_ID", Addr="37", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 2", ShortName="P1_METER_ID", Addr="38", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 3", ShortName="P1_METER_ID", Addr="39", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 4", ShortName="P1_METER_ID", Addr="40", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 5", ShortName="P1_METER_ID", Addr="41", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")

    elif meterType == "GAS_DIRECT":
        # Direct Mount Gas on port 1
        gen_xml.add_entry(PropName="P1 Direct Mount Gas Meter Type (LSB)", ShortName="P1_METER_TYPE", Addr="28",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P1 Direct Mount Gas Meter Type (MSB)", ShortName="P1_METER_TYPE", Addr="29",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P1 Pulse Ratio (LSB)", ShortName="P1_PHF", Addr="30", Mode="ca", Value="1",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")
        gen_xml.add_entry(PropName="P1 Pulse Ratio (MSB)", ShortName="P1_PHF", Addr="31", Mode="ca", Value="0",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")

        gen_xml.add_entry(PropName="P1 Pulse High Time", ShortName="P1_PULSE_HIGH_TIME", Addr="32", Mode="ca",
                          Value="10", Size="1", internalOnly=False,
                          Description="setting for pulse trail debounce 0-127 (*8192us)")

        gen_xml.add_entry(PropName="P1 Pulse Low Time", ShortName="P1_PULSE_LOW_TIME", Addr="33", Mode="ca",
                          Value="15", Size="1", internalOnly=False,
                          Description="setting for pulse debounce routine 0-15 (*1024us)")

        gen_xml.add_entry(PropName="P1 Port Config", ShortName="P1_PORT_CONFIG", Addr="34", Mode="ca",
                          Value="180", Size="1", internalOnly=False, Description="Port1 attached device and settings.")

        gen_xml.add_entry(PropName="P1 MODE2 Config", ShortName="P1_MODE2_FLAGS", Addr="35", Mode="ca",
                          Value=P1_MODE2, Size="1", internalOnly=False, Description="Port1 Gas Flags.")

        gen_xml.add_entry(PropName="P1 Meter ID Byte 0", ShortName="P1_METER_ID", Addr="36", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 1", ShortName="P1_METER_ID", Addr="37", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 2", ShortName="P1_METER_ID", Addr="38", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 3", ShortName="P1_METER_ID", Addr="39", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 4", ShortName="P1_METER_ID", Addr="40", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 5", ShortName="P1_METER_ID", Addr="41", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")

    elif meterType == "REMOTE_GAS_PULSE":
        # Remote Gas Pulse on port 1
        gen_xml.add_entry(PropName="P1 Remote Mount Gas Pulse Meter (LSB)", ShortName="P1_METER_TYPE", Addr="28",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P1 Remote Mount Gas Pulse Meter (MSB)", ShortName="P1_METER_TYPE", Addr="29",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P1 Pulse Ratio (LSB)", ShortName="P1_PHF", Addr="30", Mode="ca", Value="1",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")
        gen_xml.add_entry(PropName="P1 Pulse Ratio (MSB)", ShortName="P1_PHF", Addr="31", Mode="ca", Value="0",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")

        gen_xml.add_entry(PropName="P1 Pulse High Time", ShortName="P1_PULSE_HIGH_TIME", Addr="32", Mode="ca",
                          Value="10", Size="1", internalOnly=False,
                          Description="setting for pulse trail debounce 0-127 (*8192us)")

        gen_xml.add_entry(PropName="P1 Pulse Low Time", ShortName="P1_PULSE_LOW_TIME", Addr="33", Mode="ca",
                          Value="15", Size="1", internalOnly=False,
                          Description="setting for pulse debounce routine 0-15 (*1024us)")

        gen_xml.add_entry(PropName="P1 Port Config", ShortName="P1_PORT_CONFIG", Addr="34", Mode="ca",
                          Value="212", Size="1", internalOnly=False, Description="Port1 attached device and settings.")

        gen_xml.add_entry(PropName="P1 MODE2 Config", ShortName="P1_MODE2_FLAGS", Addr="35", Mode="ca",
                          Value=P1_MODE2, Size="1", internalOnly=False, Description="Port1 Gas Flags.")

        gen_xml.add_entry(PropName="P1 Meter ID Byte 0", ShortName="P1_METER_ID", Addr="36", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 1", ShortName="P1_METER_ID", Addr="37", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 2", ShortName="P1_METER_ID", Addr="38", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 3", ShortName="P1_METER_ID", Addr="39", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 4", ShortName="P1_METER_ID", Addr="40", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 5", ShortName="P1_METER_ID", Addr="41", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")

    else:
        # Nothing on port 1 (make it look like a disabled pulse meter)
        gen_xml.add_entry(PropName="P1 Pulse Meter Type (LSB)", ShortName="P1_METER_TYPE", Addr="28", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P1 Pulse Meter Type (MSB)", ShortName="P1_METER_TYPE", Addr="29", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P1 Pulse Ratio (LSB)", ShortName="P1_PHF", Addr="30", Mode="ca", Value="0",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")
        gen_xml.add_entry(PropName="P1 Pulse Ratio (MSB)", ShortName="P1_PHF", Addr="31", Mode="ca", Value="0",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")

        gen_xml.add_entry(PropName="P1 Pulse High Time", ShortName="P1_PULSE_HIGH_TIME", Addr="32", Mode="ca",
                          Value="0", Size="1", internalOnly=False,
                          Description="setting for pulse trail debounce 0-127 (*8192us)")

        gen_xml.add_entry(PropName="P1 Pulse Low Time", ShortName="P1_PULSE_LOW_TIME", Addr="33", Mode="ca",
                          Value="0", Size="1", internalOnly=False,
                          Description="setting for pulse debounce routine 0-15 (*1024us)")

        gen_xml.add_entry(PropName="P1 Port Config", ShortName="P1_PORT_CONFIG", Addr="34", Mode="ca",
                          Value="0", Size="1", internalOnly=False, Description="Port1 attached device and settings.")

        gen_xml.add_entry(PropName="P1 Meter ID Byte 0", ShortName="P1_METER_ID", Addr="36", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 1", ShortName="P1_METER_ID", Addr="37", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 2", ShortName="P1_METER_ID", Addr="38", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 3", ShortName="P1_METER_ID", Addr="39", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 4", ShortName="P1_METER_ID", Addr="40", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P1 Meter ID Byte 5", ShortName="P1_METER_ID", Addr="41", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")   

    gen_xml.add_entry(PropName="Update Message Range (LSB)", ShortName="updateMsgRange", Addr="42", Mode="ca",
                      Value="192", Size="2", internalOnly=False,
                      Description="max number of seconds to delay update messages")
    gen_xml.add_entry(PropName="Update Message Range (MSB)", ShortName="updateMsgRange", Addr="43", Mode="ca",
                      Value="168", Size="2", internalOnly=False,
                      Description="max number of seconds to delay update messages")

    if (portType == "0PORT" and hasRDD):
        # a standalone RDD is always installed on PORT2
        gen_xml.add_entry(PropName="PORTS_ENABLE", ShortName="PORTS_ENABLE", Addr="44", Mode="ca", Value="2",
                          Size="1", internalOnly=False, Description="Bitfield of enabled ports.")        
    elif ((portType == "1PORT") and not hasRDD):
        gen_xml.add_entry(PropName="PORTS_ENABLE", ShortName="PORTS_ENABLE", Addr="44", Mode="ca", Value="1",
                          Size="1", internalOnly=False, Description="Bitfield of enabled ports.")
    elif ((portType == "1PORT") and hasRDD) or (portType == "2PORT"):
        gen_xml.add_entry(PropName="PORTS_ENABLE", ShortName="PORTS_ENABLE", Addr="44", Mode="ca", Value="3",
                          Size="1", internalOnly=False, Description="Bitfield of enabled ports.")
    else:
        raise Exception("Invalid PORTS_ENABLE setting")

    if hasRDD:
        # RDD on port 2
        gen_xml.add_entry(PropName="P2 RDD Type (LSB)", ShortName="P2_METER_TYPE", Addr="48", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P2 RDD Type (MSB)", ShortName="P2_METER_TYPE", Addr="49", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P2 RDD Physical Port", ShortName="P2_RDD_PHYS_PORT", Addr="50", Mode="ca",
                          Value="2", Size="1", internalOnly=False, Description="Port the RDD is installed on.")

        if HW_VER == 0:
            gen_xml.add_entry(PropName="P2 Port Init Options", ShortName="P2_portInitOptions", Addr="51", Mode="ca", Value="5", Size="1", internalOnly=False, Description="Port 2 Initialization Options")
        else:
            gen_xml.add_entry(PropName="P2 Port Init Options", ShortName="P2_portInitOptions", Addr="51", Mode="ca", Value="15", Size="1", internalOnly=False, Description="Port 2 Initialization Options")

        gen_xml.add_entry(PropName="P2 RDD Periodic Query Interval (LSB)", ShortName="P2_RDD_QUERY_INTERVAL",
                          Addr="52", Mode="ca", Value="96", Size="2", internalOnly=False,
                          Description="Minutes between periodic RDD status query and transmit.")
        gen_xml.add_entry(PropName="P2 RDD Periodic Query Interval (MSB)", ShortName="P2_RDD_QUERY_INTERVAL",
                          Addr="53", Mode="ca", Value="39", Size="2", internalOnly=False,
                          Description="Minutes between periodic RDD status query and transmit.")

        gen_xml.add_entry(PropName="P2 Port Config", ShortName="P2_PORT_CONFIG", Addr="54", Mode="ca",
                          Value="128", Size="1", internalOnly=False, Description="Port2 attached device and settings.")

    elif (portType == "1PORT") and not hasRDD:
        # Nothing on port 2 (make it look like a disabled pulse meter)
        gen_xml.add_entry(PropName="P2 Pulse Meter Type (LSB)", ShortName="P2_METER_TYPE", Addr="48", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P2 Pulse Meter Type (MSB)", ShortName="P2_METER_TYPE", Addr="49", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P2 Pulse Ratio (LSB)", ShortName="P2_PHF", Addr="50", Mode="ca", Value="0",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")
        gen_xml.add_entry(PropName="P2 Pulse Ratio (MSB)", ShortName="P2_PHF", Addr="51", Mode="ca", Value="0",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")

        gen_xml.add_entry(PropName="P2 Pulse High Time", ShortName="P2_PULSE_HIGH_TIME", Addr="52", Mode="ca",
                          Value="0", Size="1", internalOnly=False,
                          Description="setting for pulse trail debounce 0-127 (*8192us)")

        gen_xml.add_entry(PropName="P2 Pulse Low Time", ShortName="P2_PULSE_LOW_TIME", Addr="53", Mode="ca",
                          Value="0", Size="1", internalOnly=False,
                          Description="setting for pulse debounce routine 0-15 (*1024us)")

        gen_xml.add_entry(PropName="P2 Port Config", ShortName="P2_PORT_CONFIG", Addr="54", Mode="ca",
                          Value="0", Size="1", internalOnly=False, Description="Port1 attached device and settings.")

        gen_xml.add_entry(PropName="P2 Meter ID Byte 0", ShortName="P2_METER_ID", Addr="56", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 1", ShortName="P2_METER_ID", Addr="57", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 2", ShortName="P2_METER_ID", Addr="58", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 3", ShortName="P2_METER_ID", Addr="59", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 4", ShortName="P2_METER_ID", Addr="60", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 5", ShortName="P2_METER_ID", Addr="61", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")     

    elif (portType == "2PORT") and (meterType == "PULSE"):
        # Pulse on port 2
        gen_xml.add_entry(PropName="P2 Pulse Meter Type (LSB)", ShortName="P2_METER_TYPE", Addr="48", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P2 Pulse Meter Type (MSB)", ShortName="P2_METER_TYPE", Addr="49", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P2 Pulse Ratio (LSB)", ShortName="P2_PHF", Addr="50", Mode="ca", Value="1",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")
        gen_xml.add_entry(PropName="P2 Pulse Ratio (MSB)", ShortName="P2_PHF", Addr="51", Mode="ca", Value="0",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")

        gen_xml.add_entry(PropName="P2 Pulse High Time", ShortName="P2_PULSE_HIGH_TIME", Addr="52", Mode="ca",
                          Value="10", Size="1", internalOnly=False,
                          Description="setting for pulse trail debounce 0-127 (*8192us)")

        gen_xml.add_entry(PropName="P2 Pulse Low Time", ShortName="P2_PULSE_LOW_TIME", Addr="53", Mode="ca",
                          Value="15", Size="1", internalOnly=False,
                          Description="setting for pulse debounce routine 0-15 (*1024us)")

        gen_xml.add_entry(PropName="P2 Port Config", ShortName="P2_PORT_CONFIG", Addr="54", Mode="ca",
                          Value="212", Size="1", internalOnly=False, Description="Port1 attached device and settings.")

        gen_xml.add_entry(PropName="P2 Meter ID Byte 0", ShortName="P2_METER_ID", Addr="56", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 1", ShortName="P2_METER_ID", Addr="57", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 2", ShortName="P2_METER_ID", Addr="58", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 3", ShortName="P2_METER_ID", Addr="59", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 4", ShortName="P2_METER_ID", Addr="60", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 5", ShortName="P2_METER_ID", Addr="61", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")

    elif (portType == "2PORT") and (meterType == "ENCODER"):
        # Encoder on port 2
        gen_xml.add_entry(PropName="P2 Encoder Meter Type (LSB)", ShortName="P2_METER_TYPE", Addr="48",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P2 Encoder Meter Type (MSB)", ShortName="P2_METER_TYPE", Addr="49",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P2 Encoder Type", ShortName="P2_ENCODER_TYPE", Addr="50", Mode="ca",
                          Value="1", Size="1", internalOnly=False, Description="Port 2 Encoder Type.")

        if HW_VER == 0:
            gen_xml.add_entry(PropName="P2 Port Init Options", ShortName="P2_portInitOptions", Addr="51", Mode="ca", Value="5", Size="1", internalOnly=False, Description="Port 2 Initialization Options")
        else:
            gen_xml.add_entry(PropName="P2 Port Init Options", ShortName="P2_portInitOptions", Addr="51", Mode="ca", Value="13", Size="1", internalOnly=False, Description="Port 2 Initialization Options")

        gen_xml.add_entry(PropName="P2 Encoder Number of Digits", ShortName="P2_ENCODER_NUM_DIGITS", Addr="52",
                          Mode="ca", Value="8", Size="1", internalOnly=False,
                          Description="Port 2 Encoder Number of Digits.")

        gen_xml.add_entry(PropName="P2 Port Config", ShortName="P2_PORT_CONFIG", Addr="54", Mode="ca",
                          Value="164", Size="1", internalOnly=False, Description="Port2 attached device and settings.")

        gen_xml.add_entry(PropName="P2 Meter ID Byte 0", ShortName="P2_METER_ID", Addr="56", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 1", ShortName="P2_METER_ID", Addr="57", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 2", ShortName="P2_METER_ID", Addr="58", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 3", ShortName="P2_METER_ID", Addr="59", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 4", ShortName="P2_METER_ID", Addr="60", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 5", ShortName="P2_METER_ID", Addr="61", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")

    elif (portType == "2PORT") and (meterType == "EXT_ALARMS"):
        # AdvAlarms/Kamstrup/Diehl on port 2
        gen_xml.add_entry(PropName="P2 E-ALM Meter Type (LSB)", ShortName="P2_METER_TYPE", Addr="48", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P2 E-ALM Meter Type (MSB)", ShortName="P2_METER_TYPE", Addr="49", Mode="ca",
                          Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P2 E-ALM Type", ShortName="P2_ENCODER_TYPE", Addr="50", Mode="ca", Value="1",
                          Size="1", internalOnly=False, Description="Port 2 E-ALM Type.")

        if HW_VER == 0:
            gen_xml.add_entry(PropName="P2 Port Init Options", ShortName="P2_portInitOptions", Addr="51", Mode="ca", Value="5", Size="1", internalOnly=False, Description="Port 2 Initialization Options")
        else:
            gen_xml.add_entry(PropName="P2 Port Init Options", ShortName="P2_portInitOptions", Addr="51", Mode="ca", Value="13", Size="1", internalOnly=False, Description="Port 2 Initialization Options")


        gen_xml.add_entry(PropName="P2 E-ALM Number of Digits", ShortName="P2_ENCODER_NUM_DIGITS", Addr="52",
                          Mode="ca", Value="8", Size="1", internalOnly=False,
                          Description="Port 2 E-ALM Number of Digits.")

        gen_xml.add_entry(PropName="P2 Port Config", ShortName="P2_PORT_CONFIG", Addr="54", Mode="ca",
                          Value="164", Size="1", internalOnly=False, Description="Port2 attached device and settings.")

        gen_xml.add_entry(PropName="P2 Meter ID Byte 0", ShortName="P2_METER_ID", Addr="56", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 1", ShortName="P2_METER_ID", Addr="57", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 2", ShortName="P2_METER_ID", Addr="58", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 3", ShortName="P2_METER_ID", Addr="59", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 4", ShortName="P2_METER_ID", Addr="60", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 5", ShortName="P2_METER_ID", Addr="61", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")

    elif (portType == "2PORT") and (meterType == "GAS_REMOTE"):
        # Remote Mount Gas on port 2
        gen_xml.add_entry(PropName="P2 Remote Mount Gas Meter Type (LSB)", ShortName="P2_METER_TYPE", Addr="48",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P2 Remote Mount Gas Meter Type (MSB)", ShortName="P2_METER_TYPE", Addr="49",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P2 Pulse Ratio (LSB)", ShortName="P2_PHF", Addr="50", Mode="ca", Value="1",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")
        gen_xml.add_entry(PropName="P2 Pulse Ratio (MSB)", ShortName="P2_PHF", Addr="51", Mode="ca", Value="0",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")

        gen_xml.add_entry(PropName="P2 Pulse High Time", ShortName="P2_PULSE_HIGH_TIME", Addr="52", Mode="ca",
                          Value="10", Size="1", internalOnly=False,
                          Description="setting for pulse trail debounce 0-127 (*8192us)")

        gen_xml.add_entry(PropName="P2 Pulse Low Time", ShortName="P2_PULSE_LOW_TIME", Addr="53", Mode="ca",
                          Value="15", Size="1", internalOnly=False,
                          Description="setting for pulse debounce routine 0-15 (*1024us)")

        gen_xml.add_entry(PropName="P2 Port Config", ShortName="P2_PORT_CONFIG", Addr="54", Mode="ca",
                          Value="212", Size="1", internalOnly=False, Description="Port1 attached device and settings.")

        gen_xml.add_entry(PropName="P2 Meter ID Byte 0", ShortName="P2_METER_ID", Addr="56", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 1", ShortName="P2_METER_ID", Addr="57", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 2", ShortName="P2_METER_ID", Addr="58", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 3", ShortName="P2_METER_ID", Addr="59", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 4", ShortName="P2_METER_ID", Addr="60", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 5", ShortName="P2_METER_ID", Addr="61", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")

    elif (portType == "2PORT") and (meterType == "GAS_DIRECT"):
        # Direct Mount Gas on port 2
        gen_xml.add_entry(PropName="P2 Direct Mount Gas Meter Type (LSB)", ShortName="P2_METER_TYPE", Addr="48",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P2 Direct Mount Gas Meter Type (MSB)", ShortName="P2_METER_TYPE", Addr="49",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P2 Pulse Ratio (LSB)", ShortName="P2_PHF", Addr="50", Mode="ca", Value="1",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")
        gen_xml.add_entry(PropName="P2 Pulse Ratio (MSB)", ShortName="P2_PHF", Addr="51", Mode="ca", Value="0",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")

        gen_xml.add_entry(PropName="P2 Pulse High Time", ShortName="P2_PULSE_HIGH_TIME", Addr="52", Mode="ca",
                          Value="10", Size="1", internalOnly=False,
                          Description="setting for pulse trail debounce 0-127 (*8192us)")

        gen_xml.add_entry(PropName="P2 Pulse Low Time", ShortName="P2_PULSE_LOW_TIME", Addr="53", Mode="ca",
                          Value="15", Size="1", internalOnly=False,
                          Description="setting for pulse debounce routine 0-15 (*1024us)")

        gen_xml.add_entry(PropName="P2 Port Config", ShortName="P2_PORT_CONFIG", Addr="54", Mode="ca",
                          Value="212", Size="1", internalOnly=False, Description="Port1 attached device and settings.")

        gen_xml.add_entry(PropName="P2 Meter ID Byte 0", ShortName="P2_METER_ID", Addr="56", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 1", ShortName="P2_METER_ID", Addr="57", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 2", ShortName="P2_METER_ID", Addr="58", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 3", ShortName="P2_METER_ID", Addr="59", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 4", ShortName="P2_METER_ID", Addr="60", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 5", ShortName="P2_METER_ID", Addr="61", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")

    elif (portType == "2PORT") and (meterType == "REMOTE_GAS_PULSE"):
        # Remote Gas Pulse on port 2
        gen_xml.add_entry(PropName="P2 Remote Mount Gas Pulse Meter (LSB)", ShortName="P2_METER_TYPE", Addr="48",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")
        gen_xml.add_entry(PropName="P2 Remote Mount Gas Pulse Meter (MSB)", ShortName="P2_METER_TYPE", Addr="49",
                          Mode="ca", Value="0", Size="2", internalOnly=False,
                          Description="meter type identifier, set by STAR Programmer")

        gen_xml.add_entry(PropName="P2 Pulse Ratio (LSB)", ShortName="P2_PHF", Addr="50", Mode="ca", Value="1",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")
        gen_xml.add_entry(PropName="P2 Pulse Ratio (MSB)", ShortName="P2_PHF", Addr="51", Mode="ca", Value="0",
                          Size="2", internalOnly=False, Description="proving hand factor 0-65535")

        gen_xml.add_entry(PropName="P2 Pulse High Time", ShortName="P2_PULSE_HIGH_TIME", Addr="52", Mode="ca",
                          Value="10", Size="1", internalOnly=False,
                          Description="setting for pulse trail debounce 0-127 (*8192us)")

        gen_xml.add_entry(PropName="P2 Pulse Low Time", ShortName="P2_PULSE_LOW_TIME", Addr="53", Mode="ca",
                          Value="15", Size="1", internalOnly=False,
                          Description="setting for pulse debounce routine 0-15 (*1024us)")

        gen_xml.add_entry(PropName="P2 Port Config", ShortName="P2_PORT_CONFIG", Addr="54", Mode="ca",
                          Value="212", Size="1", internalOnly=False, Description="Port1 attached device and settings.")

        gen_xml.add_entry(PropName="P2 Meter ID Byte 0", ShortName="P2_METER_ID", Addr="56", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 1", ShortName="P2_METER_ID", Addr="57", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 2", ShortName="P2_METER_ID", Addr="58", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 3", ShortName="P2_METER_ID", Addr="59", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 4", ShortName="P2_METER_ID", Addr="60", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")
        gen_xml.add_entry(PropName="P2 Meter ID Byte 5", ShortName="P2_METER_ID", Addr="61", Mode="ca",
                          Value="0", Size="6", internalOnly=False,
                          Description="utility's meter ID # 12 packed bcd digits")

    gen_xml.add_entry(PropName="Time alarm max dither time", ShortName="timeAlarmDither", Addr="62", Mode="ca",
                      Value="30", Size="1", internalOnly=False,
                      Description="Maximum dither of a DTC alarm or time sync range alarm after the reception of a "
                                  "timesync.")

    gen_xml.add_entry(PropName="Task Flags Byte 0", ShortName="task_flags", Addr="64", Mode="ca", Value="0",
                      Size="2", internalOnly=False, Description="each flag is cleared after its task is serviced")
    gen_xml.add_entry(PropName="Task Flags Byte 1", ShortName="task_flags", Addr="65", Mode="ca", Value="0",
                      Size="2", internalOnly=False, Description="each flag is cleared after its task is serviced")

    gen_xml.add_entry(PropName="Status Flags Byte 0", ShortName="status_flags", Addr="70", Mode="no", Value="2",
                      Size="2", internalOnly=True,
                      Description="various status flags ( simply 'flags'  seen in Series3000 )")
    gen_xml.add_entry(PropName="Status Flags Byte 1", ShortName="status_flags", Addr="71", Mode="no", Value="0",
                      Size="2", internalOnly=True,
                      Description="various status flags ( simply 'flags'  seen in Series3000 )")

    gen_xml.add_entry(PropName="Days between histogram transmissions",
                      ShortName="daysBetweenHistogramTransmissions", Addr="73", Mode="ca", Value="30", Size="1",
                      internalOnly=False,
                      Description="The MTU should send out histogram statistics message every X days, where X is this "
                                  "param.")

    gen_xml.add_entry(PropName="RTC Byte 0", ShortName="rtc", Addr="74", Mode="no", Value="0", Size="6",
                      internalOnly=True, Description="")
    gen_xml.add_entry(PropName="RTC Byte 1", ShortName="rtc", Addr="75", Mode="no", Value="0", Size="6",
                      internalOnly=True, Description="")
    gen_xml.add_entry(PropName="RTC Byte 2", ShortName="rtc", Addr="76", Mode="no", Value="0", Size="6",
                      internalOnly=True, Description="")
    gen_xml.add_entry(PropName="RTC Byte 3", ShortName="rtc", Addr="77", Mode="no", Value="1", Size="6",
                      internalOnly=True, Description="")
    gen_xml.add_entry(PropName="RTC Byte 4", ShortName="rtc", Addr="78", Mode="no", Value="1", Size="6",
                      internalOnly=True, Description="")
    gen_xml.add_entry(PropName="RTC Byte 5", ShortName="rtc", Addr="79", Mode="no", Value="0", Size="6",
                      internalOnly=True, Description="")

    gen_xml.add_entry(PropName="P1 Reading Byte 0", ShortName="p1", Addr="80", Mode="ca", Value="0", Size="8",
                      internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P1 Reading Byte 1", ShortName="p1", Addr="81", Mode="ca", Value="0", Size="8",
                      internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P1 Reading Byte 2", ShortName="p1", Addr="82", Mode="ca", Value="0", Size="8",
                      internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P1 Reading Byte 3", ShortName="p1", Addr="83", Mode="ca", Value="0", Size="8",
                      internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P1 Reading Byte 4", ShortName="p1", Addr="84", Mode="ca", Value="0", Size="8",
                      internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P1 Reading Byte 5", ShortName="p1", Addr="85", Mode="ca", Value="0", Size="8",
                      internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P1 Reading Byte 6 / Scaler Byte 0", ShortName="p1", Addr="86", Mode="ca",
                      Value="0", Size="8", internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P1 Reading Byte 7 / Scaler Byte 1", ShortName="p1", Addr="87", Mode="ca",
                      Value="0", Size="8", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="P2 Reading Byte 0", ShortName="p2", Addr="88", Mode="ca", Value="0", Size="8",
                      internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P2 Reading Byte 1", ShortName="p2", Addr="89", Mode="ca", Value="0", Size="8",
                      internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P2 Reading Byte 2", ShortName="p2", Addr="90", Mode="ca", Value="0", Size="8",
                      internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P2 Reading Byte 3", ShortName="p2", Addr="91", Mode="ca", Value="0", Size="8",
                      internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P2 Reading Byte 4", ShortName="p2", Addr="92", Mode="ca", Value="0", Size="8",
                      internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P2 Reading Byte 5", ShortName="p2", Addr="93", Mode="ca", Value="0", Size="8",
                      internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P2 Reading Byte 6 / Scaler Byte 0", ShortName="p2", Addr="94", Mode="ca",
                      Value="0", Size="8", internalOnly=False, Description="")
    gen_xml.add_entry(PropName="P2 Reading Byte 7 / Scaler Byte 1", ShortName="p2", Addr="95", Mode="ca",
                      Value="0", Size="8", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Time Since Last Time Sync", ShortName="time_since_last_time_sync", Addr="97",
                      Mode="no", Value="0", Size="1", internalOnly=True, Description="number of hours - non-rollover")

    gen_xml.add_entry(PropName="Time Sync Delta Byte 0", ShortName="time_delta", Addr="98", Mode="no", Value="0",
                      Size="4", internalOnly=True, Description="number of seconds difference from last time sync")
    gen_xml.add_entry(PropName="Time Sync Delta Byte 1", ShortName="time_delta", Addr="99", Mode="no", Value="0",
                      Size="4", internalOnly=True, Description="number of seconds difference from last time sync")
    gen_xml.add_entry(PropName="Time Sync Delta Byte 2", ShortName="time_delta", Addr="100", Mode="no",
                      Value="0", Size="4", internalOnly=True,
                      Description="number of seconds difference from last time sync")
    gen_xml.add_entry(PropName="Time Sync Delta Byte 3", ShortName="time_delta", Addr="101", Mode="no",
                      Value="0", Size="4", internalOnly=True,
                      Description="number of seconds difference from last time sync")

    gen_xml.add_entry(PropName="DCU F1 Delay", ShortName="DCU_F1_Delay", Addr="102", Mode="ca", Value="3",
                      Size="1", internalOnly=False,
                      Description="Node discovery delay for dithered DCU responses for F1 frequency")

    if meterType == "EXT_ALARMS":
        gen_xml.add_entry(PropName="E-ALM Alarm set flags", ShortName="ecoder_alarm_flags", Addr="104",
                          Mode="ca", Value="0", Size="1", internalOnly=False,
                          Description="Reserved for '-X' E-ALM Alarm set flags")

    gen_xml.add_entry(PropName="P1 Encoder Error Code", ShortName="p1_encoder_errorcode", Addr="106", Mode="ca",
                      Value="0", Size="1", internalOnly=False,
                      Description="ERROR CODE FROM ENCODER READ ROUTINE, LOCATED HERE SO IT CAN BE READ BY PROGRAMMER "
                                  "AFTER AN ENCODER REG HAS BEEN READ")

    gen_xml.add_entry(PropName="P2 Encoder Error Code", ShortName="p2_encoder_errorcode", Addr="107", Mode="ca",
                      Value="0", Size="1", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Tamper Memory (LSB)", ShortName="AlarmStatus", Addr="108", Mode="ca", Value="0",
                      Size="2", internalOnly=False,
                      Description="Used to latch alarm status, bit set to 1 when tamper is detected")
    gen_xml.add_entry(PropName="Tamper Memory (MSB)", ShortName="AlarmStatus", Addr="109", Mode="ca", Value="0",
                      Size="2", internalOnly=False,
                      Description="Used to latch alarm status, bit set to 1 when tamper is detected")

    gen_xml.add_entry(PropName="Tadiran Battery Voltage", ShortName="tadiranVoltage", Addr="110", Mode="no", Value="0",
                      Size="1", internalOnly=True, Description="Tadiran battery voltage => scaled to health message values")

    gen_xml.add_entry(PropName="Ambient Temperature", ShortName="batteryTemperature", Addr="111", Mode="no",
                      Value="0", Size="1", internalOnly=True, Description="")

    gen_xml.add_entry(PropName="Minimum Battery Voltage", ShortName="minBatteryVoltage", Addr="112", Mode="no",
                      Value="0", Size="1", internalOnly=True, Description="")

    gen_xml.add_entry(PropName="Downloading Firmware State", ShortName="dfw_state", Addr="113", Mode="v",
                      Value="0", Size="1", internalOnly=False,
                      Description="0 = Ready for Download 1 = Accepting Packets (incomplete set) 2 = Waiting for Apply "
                                  "(complete set) 3 - 65535 = Reserved")

    gen_xml.add_entry(PropName="CRC Flash Byte 0", ShortName="crc_flash", Addr="114", Mode="no", Value="0",
                      Size="2", internalOnly=True,
                      Description="program flash 'this only gets updated on command, use afc task flag in Task Flags lsb "
                                  "(addr 64) '")
    gen_xml.add_entry(PropName="CRC Flash Byte 1", ShortName="crc_flash", Addr="115", Mode="no", Value="0",
                      Size="2", internalOnly=True,
                      Description="program flash 'this only gets updated on command, use afc task flag in Task Flags lsb "
                                  "(addr 64) '")

    gen_xml.add_entry(PropName="AFC Coarse Correction Mirror", ShortName="AFC_CoarseCorrectionMirror",
                      Addr="116", Mode="no", Value="0", Size="1", internalOnly=True,
                      Description="AFC Coarse Frequency correction (mirror value) needs sign")

    gen_xml.add_entry(PropName="AFC Fine Frequency Correction", ShortName="AFC_FineFrequencyCorrection",
                      Addr="117", Mode="no", Value="0", Size="1", internalOnly=True,
                      Description="AFC Fine Frequency Correction (units of 5Hz per increment)")

    gen_xml.add_entry(PropName="AFC Sync Status", ShortName="AFC_SyncStatus", Addr="118", Mode="no", Value="255",
                      Size="1", internalOnly=True, Description="AFC Status Last Received Time Sync  0x0076 (118)")

    gen_xml.add_entry(PropName="AFC Num Freqency Error Readings Saved",
                      ShortName="AFC_NumFreqErrorReadingsSaved", Addr="119", Mode="no", Value="0", Size="1",
                      internalOnly=True, Description="AFC Num Freq Error Readings Saved")

    gen_xml.add_entry(PropName="AFC Last Freqency Error Average Byte 0", ShortName="AFC_LastFreqErrorAvg",
                      Addr="120", Mode="no", Value="0", Size="2", internalOnly=True,
                      Description="AFC Last Freq Error Avg")
    gen_xml.add_entry(PropName="AFC Last Freqency Error Average Byte 1", ShortName="AFC_LastFreqErrorAvg",
                      Addr="121", Mode="no", Value="0", Size="2", internalOnly=True,
                      Description="AFC Last Freq Error Avg")

    gen_xml.add_entry(PropName="RF RSSI of Last Received Packet", ShortName="RF_RSSIOfLastReceivedPkt",
                      Addr="122", Mode="no", Value="0", Size="1", internalOnly=True,
                      Description="units of 1 dBm per increment")

    gen_xml.add_entry(PropName="RF Frequency Error of Last Received Packet",
                      ShortName="RF_FreqErrorOfLastReceivedPkt", Addr="123", Mode="no", Value="0", Size="1",
                      internalOnly=False, Description="units of 114.4Hz per increment")

    gen_xml.add_entry(PropName="MTU or DCUID of Last Received Packet Byte 0",
                      ShortName="RF_MTUorDCUIDOfLastReceivedPkt", Addr="124", Mode="no", Value="0", Size="4",
                      internalOnly=False, Description="MTU/DCU ID of last packet received")
    gen_xml.add_entry(PropName="MTU or DCUID of Last Received Packet Byte 1",
                      ShortName="RF_MTUorDCUIDOfLastReceivedPkt", Addr="125", Mode="no", Value="0", Size="4",
                      internalOnly=False, Description="MTU/DCU ID of last packet received")
    gen_xml.add_entry(PropName="MTU or DCUID of Last Received Packet Byte 2",
                      ShortName="RF_MTUorDCUIDOfLastReceivedPkt", Addr="126", Mode="no", Value="0", Size="4",
                      internalOnly=False, Description="MTU/DCU ID of last packet received")
    gen_xml.add_entry(PropName="MTU or DCUID of Last Received Packet Byte 3",
                      ShortName="RF_MTUorDCUIDOfLastReceivedPkt", Addr="127", Mode="no", Value="0", Size="4",
                      internalOnly=False, Description="MTU/DCU ID of last packet received")

    gen_xml.add_entry(PropName="Accumulated Energy Usage Estimate 0", ShortName="LifetimeEnergyUsageEstimate",
                      Addr="128", Mode="ca", Value="0", Size="2", internalOnly=False,
                      Description="Estimate of the accumulated energy usage of the unit over its lifetime (units of mA-H). "
                                  "Updated when the health message is sent.")
    gen_xml.add_entry(PropName="Accumulated Energy Usage Estimate 1", ShortName="LifetimeEnergyUsageEstimate",
                      Addr="129", Mode="ca", Value="0", Size="2", internalOnly=False,
                      Description="Estimate of the accumulated energy usage of the unit over its lifetime (units of mA-H). "
                                  "Updated when the health message is sent.")

    gen_xml.add_entry(PropName="Reset Counter", ShortName="ResetCount", Addr="130", Mode="c", Value="0",
                      Size="1", internalOnly=False, Description="reset counter every count indicates a reset")

    gen_xml.add_entry(PropName="Power on Reset Count", ShortName="PowerOnResetCount", Addr="131", Mode="c",
                      Value="0", Size="1", internalOnly=False, Description="power on reset every count indicates por")

    gen_xml.add_entry(PropName="Last Watchdog State", ShortName="WatchdogFailTask", Addr="132", Mode="no",
                      Value="0", Size="1", internalOnly=True, Description="task that had watchdog timeout")

    gen_xml.add_entry(PropName="Last Reset Reason", ShortName="LastResetReason", Addr="133", Mode="c",
                      Value="0", Size="1", internalOnly=False, Description="Reason for Last Reset")

    gen_xml.add_entry(PropName="Last Reset Reason Date and Time Byte 0", ShortName="LastResetDateTime",
                      Addr="134", Mode="c", Value="0", Size="4", internalOnly=False,
                      Description="date/Time of Last Reset Lsb . . . .msb")
    gen_xml.add_entry(PropName="Last Reset Reason Date and Time Byte 1", ShortName="LastResetDateTime",
                      Addr="135", Mode="c", Value="0", Size="4", internalOnly=False,
                      Description="date/Time of Last Reset Lsb . . . .msb")
    gen_xml.add_entry(PropName="Last Reset Reason Date and Time Byte 2", ShortName="LastResetDateTime",
                      Addr="136", Mode="c", Value="0", Size="4", internalOnly=False,
                      Description="date/Time of Last Reset Lsb . . . .msb")
    gen_xml.add_entry(PropName="Last Reset Reason Date and Time Byte 3", ShortName="LastResetDateTime",
                      Addr="137", Mode="c", Value="0", Size="4", internalOnly=False,
                      Description="date/Time of Last Reset Lsb . . . .msb")

    gen_xml.add_entry(PropName="Battery Read Date and Time Byte 0", ShortName="battReadTimeStamp", Addr="138",
                      Mode="c", Value="0", Size="4", internalOnly=False,
                      Description="Date/Time of voltage and temperature readings")
    gen_xml.add_entry(PropName="Battery Read Date and Time Byte 1", ShortName="battReadTimeStamp", Addr="139",
                      Mode="c", Value="0", Size="4", internalOnly=False,
                      Description="Date/Time of voltage and temperature readings")
    gen_xml.add_entry(PropName="Battery Read Date and Time Byte 2", ShortName="battReadTimeStamp", Addr="140",
                      Mode="c", Value="0", Size="4", internalOnly=False,
                      Description="Date/Time of voltage and temperature readings")
    gen_xml.add_entry(PropName="Battery Read Date and Time Byte 3", ShortName="battReadTimeStamp", Addr="141",
                      Mode="c", Value="0", Size="4", internalOnly=False,
                      Description="Date/Time of voltage and temperature readings")

    gen_xml.add_entry(PropName="Energizer Low Voltage Deadband", ShortName="energizerLowVoltageDeadband", Addr="142",
                      Mode="ca", Value="10", Size="1", internalOnly=False,
                      Description="When exiting ship mode, the Energizer voltage must be this much greater than the low "
                                  "voltage threshold level.")

    gen_xml.add_entry(PropName="Energizer Low Voltage Threshold Intercept", ShortName="energizerLowVolThresholdIntercept", Addr="143",
                      Mode="ca", Value="120", Size="1", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="On Demand Request Counter 0", ShortName="OnDemandReqCounter", Addr="144",
                      Mode="ca", Value="0", Size="2", internalOnly=False,
                      Description="Count of Received OnDemand messages 0x88 CommandByte 0")
    gen_xml.add_entry(PropName="On Demand Request Counter 1", ShortName="OnDemandReqCounter", Addr="145",
                      Mode="ca", Value="0", Size="2", internalOnly=False,
                      Description="Count of Received OnDemand messages 0x88 CommandByte 0")

    gen_xml.add_entry(PropName="Coil-Over-RF Request Counter 0", ShortName="CoilOverRfReqCounter", Addr="146",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="Count of Received 0xB3 messages")
    gen_xml.add_entry(PropName="Coil-Over-RF Request Counter 1", ShortName="CoilOverRfReqCounter", Addr="147",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="Count of Received 0xB3 messages")

    gen_xml.add_entry(PropName="Number of lifetime RF update attempts", ShortName="NumAirUpdates", Addr="148",
                      Mode="ca", Value="0", Size="1", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Number of lifetime Coil update attempts", ShortName="NumCoilUpdates", Addr="149",
                      Mode="ca", Value="0", Size="1", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Number of Low Priority Data Transmissions",
                      ShortName="NumLowPriDataTransmissions", Addr="150", Mode="ca", Value="1", Size="1",
                      internalOnly=False, Description="Total number of transmissions for low priority data messages")

    gen_xml.add_entry(PropName="Tadiran Low Voltage Deadband", ShortName="tadiranLowVoltageDeadband", Addr="151", Mode="ca",
                      Value="10", Size="1", internalOnly=False,
                      Description="added to tadiran low voltage threshold level, ensures sufficient battery voltage to resume normal operations, "
                                  "this field contains the number of 10mV counts.")

    gen_xml.add_entry(PropName="Energizer Low Voltage Threshold Slope", ShortName="energizerLowVolThresholdSlope", Addr="152", Mode="ca", Value="0",
                      Size="1", internalOnly=False,
                      Description="")

    gen_xml.add_entry(PropName="STAR Inter-Packet Delay", ShortName="STARInterPacketDelay", Addr="155",
                      Mode="ca", Value="30", Size="1", internalOnly=False,
                      Description="Number of seconds between packet transmissions (the TX control for multiple packets "
                                  "that are scheduled for transmission)")

    gen_xml.add_entry(PropName="Message Retry Control Byte 0", ShortName="MessageRetryControl", Addr="156",
                      Mode="ca", Value="53", Size="2", internalOnly=False, Description="Message Retry Control")
    gen_xml.add_entry(PropName="Message Retry Control Byte 1", ShortName="MessageRetryControl", Addr="157",
                      Mode="ca", Value="15", Size="2", internalOnly=False, Description="Message Retry Control")

    gen_xml.add_entry(PropName="Response Timing Byte 0", ShortName="ResponseTiming", Addr="158", Mode="ca",
                      Value="100", Size="2", internalOnly=False, Description="Response Timing")
    gen_xml.add_entry(PropName="Response Timing Byte 1", ShortName="ResponseTiming", Addr="159", Mode="ca",
                      Value="100", Size="2", internalOnly=False, Description="Response Timing")

    gen_xml.add_entry(PropName="Secondary Response Start Delay", ShortName="SecondaryResponseStart", Addr="160",
                      Mode="ca", Value="210", Size="1", internalOnly=False, Description="Secondary Response Start")

    gen_xml.add_entry(PropName="Energizer Voltage", ShortName="energizerVoltage", Addr="161",
                      Mode="no", Value="210", Size="1", internalOnly=True, Description="Secondary Response Start")

    gen_xml.add_entry(PropName="Wake-up Listen Window", ShortName="WakeUpListenWindow", Addr="162", Mode="ca",
                      Value="25", Size="1", internalOnly=False, Description="Primary Message Rx Window")

    gen_xml.add_entry(PropName="Fast Message Configuration", ShortName="FASTMSG_Cfg", Addr="163", Mode="ca",
                      Value="0", Size="1", internalOnly=False, Description="Fast Messaging Configuration")

    gen_xml.add_entry(PropName="Window A Communication Rate Byte 0", ShortName="Window_A_CommRate", Addr="164",
                      Mode="ca", Value="180", Size="2", internalOnly=False, Description="Window A Communication Rate")
    gen_xml.add_entry(PropName="Window A Communication Rate Byte 1", ShortName="Window_A_CommRate", Addr="165",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="Window A Communication Rate")

    gen_xml.add_entry(PropName="Window A Start", ShortName="Window_A_Start", Addr="166", Mode="ca", Value="0",
                      Size="1", internalOnly=False, Description="Window A Start")

    gen_xml.add_entry(PropName="Window B Start", ShortName="Window_B_Start", Addr="167", Mode="ca", Value="0",
                      Size="1", internalOnly=False, Description="Window B Start")

    gen_xml.add_entry(PropName="Window B Communication Rate Byte 0", ShortName="Window_B_CommRate", Addr="168",
                      Mode="ca", Value="16", Size="2", internalOnly=False, Description="Window B Communication Rate")
    gen_xml.add_entry(PropName="Window B Communication Rate Byte 1", ShortName="Window_B_CommRate", Addr="169",
                      Mode="ca", Value="14", Size="2", internalOnly=False, Description="Window B Communication Rate")

    gen_xml.add_entry(PropName="Wake-up Starting Offset", ShortName="WakeUpStartingOffset", Addr="170",
                      Mode="ca", Value="51", Size="1", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="AFC Coarse Frequency Correction Master", ShortName="AFC_CoarseCorrectionMaster",
                      Addr="171", Mode="ca", Value="0", Size="1", internalOnly=False,
                      Description="Freq correction from center, Added to Fine Freq to derive total Freq correction. "
                                  "Signed integer with units = 296.63Hz")

    gen_xml.add_entry(PropName="Lifetime Number of AFC Corrections Byte 0",
                      ShortName="LifetimeNumberOfAfcCorrections", Addr="172", Mode="no", Value="0", Size="2",
                      internalOnly=False,
                      Description="How many times, over the course of the MTUs entire life, that an AFC correction has occurred.")
    gen_xml.add_entry(PropName="Lifetime Number of AFC Corrections Byte 1",
                      ShortName="LifetimeNumberOfAfcCorrections", Addr="173", Mode="no", Value="0", Size="2",
                      internalOnly=False,
                      Description="How many times, over the course of the MTUs entire life, that an AFC correction has occurred.")

    gen_xml.add_entry(PropName="Maximum RF Tx duty cycle over any 6 minute interval",
                      ShortName="FCC_6min_Limit_Percentage", Addr="175", Mode="ca",
                      Value="40", Size="1", internalOnly=False,
                      Description="Maximum RF Tx duty cycle that the MTU can maintain over any 6 minute interval. If the "
                                  "MTU would exceed this limit with any transmission it will delay the transmission until "
                                  "the transmission can take place without violating the limit.")

    gen_xml.add_entry(PropName="Antenna Type", ShortName="antennaType", Addr="177", Mode="ca", Value=antennaType,
                      Size="1", internalOnly=False,
                      Description="What type of antenna this MTU has (0 = internal bowtie, 1 = external")

    gen_xml.add_entry(PropName="Operating Frequency Adjustment[0]", ShortName="operatingFrequencyAdjustment",
                      Addr="178", Mode="no", Value="0", Size="2", internalOnly=False, Description="")
    gen_xml.add_entry(PropName="Operating Frequency Adjustment[1]", ShortName="operatingFrequencyAdjustment",
                      Addr="179", Mode="no", Value="0", Size="2", internalOnly=False, Description="")

    if mtuRange == "STD":
        gen_xml.add_entry(PropName="Power Amp Temperature Coefficient",
                          ShortName="PowerAmpTemperatureCoefficient", Addr="180", Mode="ca", Value="31", Size="1",
                          internalOnly=False,
                          Description="Adjustment to the NominalPowerAmpPWMDutyCycle based on temperature "
                                      "(see EnablePowerAmplifierOutpoutPowerReferencePWM())")

        gen_xml.add_entry(PropName="Power Amp Frequency Coefficient", ShortName="PowerAmpFrequencyCoefficient",
                          Addr="181", Mode="ca", Value="120", Size="1", internalOnly=False,
                          Description="Adjustment to the NominalPowerAmpPWMDutyCycle based on frequency "
                                      "(see EnablePowerAmplifierOutpoutPowerReferencePWM())")

    elif mtuRange == "EXT":
        gen_xml.add_entry(PropName="Power Amp Temperature Coefficient",
                          ShortName="PowerAmpTemperatureCoefficient", Addr="180", Mode="ca", Value="34", Size="1",
                          internalOnly=False,
                          Description="Adjustment to the NominalPowerAmpPWMDutyCycle based on temperature "
                                      "(see EnablePowerAmplifierOutpoutPowerReferencePWM())")

        gen_xml.add_entry(PropName="Power Amp Frequency Coefficient", ShortName="PowerAmpFrequencyCoefficient",
                          Addr="181", Mode="ca", Value="68", Size="1", internalOnly=False,
                          Description="Adjustment to the NominalPowerAmpPWMDutyCycle based on frequency "
                                      "(see EnablePowerAmplifierOutpoutPowerReferencePWM())")

    gen_xml.add_entry(PropName="Extended Range Nominal Power Amp PWM Duty Cycle Byte 0",
                      ShortName="ExtendedRangePowerAmpPWMDutyCycle", Addr="182", Mode="no", Value="11", Size="2",
                      internalOnly=False,
                      Description="Extended Range Nominal Duty cycle at 460MHz and 15C (in units of 0.1% of the PWM which sets the "
                                  "PA output power control loop reference voltage)")
    gen_xml.add_entry(PropName="Extended Range Nominal Power Amp PWM Duty Cycle Byte 1",
                      ShortName="ExtendedRangePowerAmpPWMDutyCycle", Addr="183", Mode="no", Value="1", Size="2",
                      internalOnly=False,
                      Description="Extended Range Nominal Duty cycle at 460MHz and 15C (in units of 0.1% of the PWM which sets the "
                                  "PA output power control loop reference voltage)")

    gen_xml.add_entry(PropName="Lifetime XCVR Init Count 0", ShortName="LifetimeXCVRInitCount", Addr="184",
                      Mode="no", Value="0", Size="4", internalOnly=True,
                      Description="Number of times the transceiver has been initialized over the lifetime of the product")
    gen_xml.add_entry(PropName="Lifetime XCVR Init Count 1", ShortName="LifetimeXCVRInitCount", Addr="185",
                      Mode="no", Value="0", Size="4", internalOnly=True,
                      Description="Number of times the transceiver has been initialized over the lifetime of the product")
    gen_xml.add_entry(PropName="Lifetime XCVR Init Count 2", ShortName="LifetimeXCVRInitCount", Addr="186",
                      Mode="no", Value="0", Size="4", internalOnly=True,
                      Description="Number of times the transceiver has been initialized over the lifetime of the product")
    gen_xml.add_entry(PropName="Lifetime XCVR Init Count 3", ShortName="LifetimeXCVRInitCount", Addr="187",
                      Mode="no", Value="0", Size="4", internalOnly=True,
                      Description="Number of times the transceiver has been initialized over the lifetime of the product")

    gen_xml.add_entry(PropName="Si4460 Chip Level AFC Limiter Byte 0", ShortName="Si446xAfcLimiterSetting",
                      Addr="188", Mode="ca", Value="29", Size="2", internalOnly=False,
                      Description="Amount the transceiver should allow the chip level AFC unit to deviate from the center "
                                  "frequency in its search for a received signal (units of 28.6Hz per increment)")
    gen_xml.add_entry(PropName="Si4460 Chip Level AFC Limiter Byte 1", ShortName="Si446xAfcLimiterSetting",
                      Addr="189", Mode="ca", Value="11", Size="2", internalOnly=False,
                      Description="Amount the transceiver should allow the chip level AFC unit to deviate from the center "
                                  "frequency in its search for a received signal (units of 28.6Hz per increment)")

    if mtuRange == "STD":
        if internalAntenna:
            gen_xml.add_entry(PropName="Si446x Tx Config",
                              ShortName="Si446xTxConfig", Addr="190", Mode="ca", Value="2", Size="1",
                              internalOnly=False,
                              Description="Options pertaining to the TX configuration of the transmitter")
        else:
            gen_xml.add_entry(PropName="Si446x Tx Config",
                              ShortName="Si446xTxConfig", Addr="190", Mode="ca", Value="0", Size="1",
                              internalOnly=False,
                              Description="Options pertaining to the TX configuration of the transmitter")
    elif mtuRange == "EXT":
        if internalAntenna:
            gen_xml.add_entry(PropName="Si446x Tx Config",
                              ShortName="Si446xTxConfig", Addr="190", Mode="ca", Value="3", Size="1",
                              internalOnly=False,
                              Description="Options pertaining to the TX configuration of the transmitter")
        else:
            gen_xml.add_entry(PropName="Si446x Tx Config",
                              ShortName="Si446xTxConfig", Addr="190", Mode="ca", Value="1", Size="1",
                              internalOnly=False,
                              Description="Options pertaining to the TX configuration of the transmitter")

    gen_xml.add_entry(PropName="Si4460 PA Level", ShortName="Si446xPAPowerLevelSetting", Addr="191", Mode="ca",
                      Value="10", Size="1", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Immediate Alarm Configuration (LSB)", ShortName="ImmediateAlarmConfig", Addr="192",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="")
    gen_xml.add_entry(PropName="Immediate Alarm Configuration (MSB)", ShortName="ImmediateAlarmConfig", Addr="193",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Alarm Transmit Configuration (LSB)", ShortName="AlarmTransmitConfig", Addr="194",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="")
    gen_xml.add_entry(PropName="Alarm Transmit Configuration (MSB)", ShortName="AlarmTransmitConfig", Addr="195",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Health Message Period", ShortName="healthMessagePeriod", Addr="196", Mode="ca",
                      Value="7", Size="1", internalOnly=False,
                      Description="How often the Health Message is transmitted. Units are in days, "
                                  "1 = transmitted each day, 30 = transmitted once each 30 days.")

    gen_xml.add_entry(PropName="Time Difference Alarm", ShortName="time_difference_alarm", Addr="197", Mode="ca",
                      Value="120", Size="1", internalOnly=False,
                      Description="number of sec difference between MTU time and DCU time sync to send alarm")

    gen_xml.add_entry(PropName="Daily Read Hour", ShortName="daily_read_hour", Addr="198", Mode="ca",
                      Value="255", Size="1", internalOnly=False,
                      Description="[0..23] Hour at which the daily read is taken and transmitted. "
                                  "Values other than [0..23] signify that daily reads are disabled.")

    if meterType == "EXT_ALARMS":
        gen_xml.add_entry(PropName="E-ALM Alarm Enable Flags", ShortName="ecoder_alarm_enable_flags", Addr="200",
                          Mode="ca", Value="0", Size="1", internalOnly=False,
                          Description="Reserved for E-ALM Alarm Enable Flags")

    gen_xml.add_entry(PropName="Standard Range Nominal Power Amp PWM Duty Cycle Byte 0",
                      ShortName="StandardRangePowerAmpPWMDutyCycle", Addr="202", Mode="no", Value="132", Size="2",
                      internalOnly=False,
                      Description="Standard Range Nominal Duty cycle at 460MHz and 15C (in units of 0.1% of the PWM which sets the "
                                  "PA output power control loop reference voltage)")
    gen_xml.add_entry(PropName="Standard Range Nominal Power Amp PWM Duty Cycle Byte 1",
                      ShortName="StandardRangePowerAmpPWMDutyCycle", Addr="203", Mode="no", Value="0", Size="2",
                      internalOnly=False,
                      Description="Standard Range Nominal Duty cycle at 460MHz and 15C (in units of 0.1% of the PWM which sets the "
                                  "PA output power control loop reference voltage)")

    if meterType == "ENCODER":
        gen_xml.add_entry(PropName="Port 1 Encoder Digits to Drop", ShortName="P1_Encoder_DigitsToDrop",
                          Addr="204",
                          Mode="ca", Value="0", Size="1", internalOnly=False, Description="")

        gen_xml.add_entry(PropName="Port 2 Encoder Digits to Drop", ShortName="P2_Encoder_DigitsToDrop",
                          Addr="205",
                          Mode="ca", Value="0", Size="1", internalOnly=False, Description="")
        
    gen_xml.add_entry(PropName="Max Flow 0", ShortName="MAX_FLOW", Addr="206", Mode="ca", Value="250", Size="2",
                      internalOnly=False,
                      Description="MAX_FLOW_ADDR = MAX_FLOW_DEF MAX FLOW between reading used to verify reading 0 - 32767")
    gen_xml.add_entry(PropName="Max Flow 1", ShortName="MAX_FLOW", Addr="207", Mode="ca", Value="0", Size="2",
                      internalOnly=False,
                      Description="MAX_FLOW_ADDR = MAX_FLOW_DEF MAX FLOW between reading used to verify reading 0 - 32767")

    gen_xml.add_entry(PropName="Battery Life Events Message Interval", ShortName="batteryLifeEventsMessagePeriod",
                      Addr="238", Mode="ca", Value="30", Size="1", internalOnly=False,
                      Description="Specifies the interval in which battery life events are transmitted (units of days).")

    if (meterType == "EXT_ALARMS") and (portType == "2PORT"):
        gen_xml.add_entry(PropName="Desired Alarm Message Version/Variant", ShortName="alarmVersionVariant",
                          Addr="239", Mode="ca", Value="2", Size="1", internalOnly=False,
                          Description="Specifies what variants of the alarm messages the MTU should send.")
    else:
        gen_xml.add_entry(PropName="Desired Alarm Message Version/Variant", ShortName="alarmVersionVariant",
                          Addr="239", Mode="ca", Value="0", Size="1", internalOnly=False,
                          Description="Specifies what variants of the alarm messages the MTU should send.")

    gen_xml.add_entry(PropName="MTU Revision Build Byte 0", ShortName="MTU_FW_build", Addr="240", Mode="va",
                      Value=str(bldLSB), Size="2", internalOnly=False,
                      Description="MTU firmware version Build lsb , msb")
    gen_xml.add_entry(PropName="MTU Revision Build Byte 1", ShortName="MTU_FW_build", Addr="241", Mode="va",
                      Value=str(bldMSB), Size="2", internalOnly=False,
                      Description="MTU firmware version Build lsb , msb")

    gen_xml.add_entry(PropName="Detected HW ID", ShortName="DetectedHwId", Addr="242", Mode="va", Value=str(HW_VER),
                      Size="1", internalOnly=False, Description="HW ID as detected by the firmware. Should match HW_VER.")

    gen_xml.add_entry(PropName="MTU Hardware Version", ShortName="HW_VER", Addr="243", Mode="ca", Value=str(HW_VER),
                      Size="1", internalOnly=False, Description="MTU hardware version as set by the XML file. Should match DetectedHwId.")

    gen_xml.add_entry(PropName="MTU Software Version Format", ShortName="MTU_FW_ver_Format", Addr="244",
                      Mode="va", Value="254", Size="1", internalOnly=False,
                      Description="MTU firmware version format flag")

    gen_xml.add_entry(PropName="MTU FW Revision Major", ShortName="MTU_FW_major_revision", Addr="245", Mode="va",
                      Value=str(major), Size="1", internalOnly=False, Description="MTU firmware version Major")

    gen_xml.add_entry(PropName="MTU FW Revision Minor", ShortName="MTU_FW_minor_revision", Addr="246", Mode="va",
                      Value=str(minor), Size="1", internalOnly=False, Description="MTU firmware version Minor")

    gen_xml.add_entry(PropName="Network ID (LSB)", ShortName="networkId", Addr="248", Mode="ca", Value="0",
                      Size="2", internalOnly=False, Description="Network Id")
    gen_xml.add_entry(PropName="Network ID (MSB)", ShortName="networkId", Addr="249", Mode="ca", Value="0",
                      Size="2", internalOnly=False, Description="Network Id")

    gen_xml.add_entry(PropName="Moisture Detect Sampling Interval Byte 0",
                      ShortName="MoistureDetectSamplingInterval", Addr="250", Mode="ca", Value="90", Size="2",
                      internalOnly=False,
                      Description="Frequency at which a moisture reading should be taken (10s per increment). "
                                  "A value of 0 disables sampling.")
    gen_xml.add_entry(PropName="Moisture Detect Sampling Interval Byte 1",
                      ShortName="MoistureDetectSamplingInterval", Addr="251", Mode="ca", Value="0", Size="2",
                      internalOnly=False,
                      Description="Frequency at which a moisture reading should be taken (10s per increment). "
                                  "A value of 0 disables sampling.")

    gen_xml.add_entry(PropName="Last Moisture Detect Reading Byte 0", ShortName="LastMoistureDetectReading",
                      Addr="252", Mode="c", Value="0", Size="2", internalOnly=False,
                      Description="This location is populated whenever a moisture detect reading is taken. "
                                  "It is the voltage on the MOISTURE_SNS line, as a percentage of VCC. "
                                  "(Units of 0.01% per increment)Nominally (when no moisture is present) "
                                  "this will have a value of about 5000 (50.00%)")
    gen_xml.add_entry(PropName="Last Moisture Detect Reading Byte 1", ShortName="LastMoistureDetectReading",
                      Addr="253", Mode="c", Value="0", Size="2", internalOnly=False,
                      Description="This location is populated whenever a moisture detect reading is taken. "
                                  "It is the voltage on the MOISTURE_SNS line, as a percentage of VCC. "
                                  "(Units of 0.01% per increment)Nominally (when no moisture is present) "
                                  "this will have a value of about 5000 (50.00%)")

    gen_xml.add_entry(PropName="Message Tag Tracker Reset Request", ShortName="messageTagTrackerResetRequest",
                      Addr="255", Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="Set to 1 (over an authenticated coil session) to cause the MTU to reset its message "
                                  "tag tracking (and accept the next message it receives)")

    gen_xml.add_entry(PropName="Encryption Flags", ShortName="EncryptionFlags", Addr="318", Mode="va", Value="0",
                      Size="1", internalOnly=False,
                      Description="Encryption Flags read only bit 0 indicates encryption enabled Set: enabled Clear: "
                                  "disabled")

    gen_xml.add_entry(PropName="Key Index", ShortName="KeyIndex", Addr="319", Mode="no", Value="0", Size="1",
                      internalOnly=False, Description="Key Index 'read only'")

    gen_xml.add_entry(PropName="Lifetime Encoder Read Count 0", ShortName="LifetimeEncoderReadCount", Addr="320",
                      Mode="ca", Value="0", Size="4", internalOnly=False,
                      Description="Number of times the encoder has been read over the lifetime of the product")
    gen_xml.add_entry(PropName="Lifetime Encoder Read Count 1", ShortName="LifetimeEncoderReadCount", Addr="321",
                      Mode="ca", Value="0", Size="4", internalOnly=False,
                      Description="Number of times the encoder has been read over the lifetime of the product")
    gen_xml.add_entry(PropName="Lifetime Encoder Read Count 2", ShortName="LifetimeEncoderReadCount", Addr="322",
                      Mode="ca", Value="0", Size="4", internalOnly=False,
                      Description="Number of times the encoder has been read over the lifetime of the product")
    gen_xml.add_entry(PropName="Lifetime Encoder Read Count 3", ShortName="LifetimeEncoderReadCount", Addr="323",
                      Mode="ca", Value="0", Size="4", internalOnly=False,
                      Description="Number of times the encoder has been read over the lifetime of the product")

    gen_xml.add_entry(PropName="Alarm Enable Configuration (LSB)", ShortName="AlarmEnableConfig", Addr="326",
                      Mode="ca", Value="255", Size="2", internalOnly=False, Description="")
    
    # Note: following values should work but may want/need to be more selective in future
    if MTUType == 201:
        # DUG does not support cut wire
        gen_xml.add_entry(PropName="Alarm Enable Configuration (MSB)", ShortName="AlarmEnableConfig", Addr="327",
                          Mode="ca", Value="119", Size="2", internalOnly=False, Description="")
    else:
        gen_xml.add_entry(PropName="Alarm Enable Configuration (MSB)", ShortName="AlarmEnableConfig", Addr="327",
                          Mode="ca", Value="255", Size="2", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Statistics Message Min dither byte 0", ShortName="statMsgMinDither", Addr="328",
                      Mode="ca", Value="132", Size="2", internalOnly=False,
                      Description="")
    gen_xml.add_entry(PropName="Statstics Message Min dither byte 1", ShortName="statMsgMinDither", Addr="329",
                      Mode="ca", Value="3", Size="2", internalOnly=False,
                      Description="")

    gen_xml.add_entry(PropName="Statstics Message Max dither byte 0", ShortName="statMsgMaxDither", Addr="330",
                      Mode="ca", Value="96", Size="2", internalOnly=False,
                      Description="")
    gen_xml.add_entry(PropName="Statstics Message Max dither byte 1", ShortName="statMsgMaxDither", Addr="331",
                      Mode="ca", Value="84", Size="2", internalOnly=False,
                      Description="")

    gen_xml.add_entry(PropName="RF LBT Threshold (dBm)", ShortName="RF_LBT_TX_THRESHOLD_DBM", Addr="332",
                      Mode="ca", Value="196", Size="1", internalOnly=False,
                      Description="Value use for the LBT algorith to determine if channel signal strength is strong enough to warrant delaying a transmission.")

    gen_xml.add_entry(PropName="Tadiran Low Voltage Threshold Slope", ShortName="tadiranLowVolThresholdSlope", Addr="333",
                      Mode="ca", Value="0", Size="1", internalOnly=False,
                      Description="")    
    
    gen_xml.add_entry(PropName="Tadiran Low Voltage Threshold Intercept", ShortName="tadiranLowVolThresholdIntercept", Addr="334",
                      Mode="ca", Value="130", Size="1", internalOnly=False,
                      Description="")
    
    gen_xml.add_entry(PropName="Tadiran Shutdown Voltage Threshold Slope", ShortName="tadiranShdnVolThresholdSlope", Addr="335",
                      Mode="ca", Value="0", Size="1", internalOnly=False,
                      Description="")
    
    gen_xml.add_entry(PropName="Tadiran Shutdown Voltage Threshold Intercept", ShortName="tadiranShdnVolThresholdIntercept", Addr="336",
                      Mode="ca", Value="90", Size="1", internalOnly=False,
                      Description="")
    
    gen_xml.add_entry(PropName="Energizer Shutdown Voltage Threshold Slope", ShortName="energizerShdnVolThresholdSlope", Addr="337",
                      Mode="ca", Value="0", Size="1", internalOnly=False,
                      Description="")
    
    gen_xml.add_entry(PropName="Energizer Shutdown Voltage Threshold Intercept", ShortName="energizerShdnVolThresholdIntercept", Addr="338",
                      Mode="ca", Value="40", Size="1", internalOnly=False,
                      Description="")
    
    gen_xml.add_entry(PropName="Energizer Sag Voltage Threshold Slope", ShortName="energizerSagVolThresholdSlope", Addr="339",
                      Mode="ca", Value="0", Size="1", internalOnly=False,
                      Description="")
    
    gen_xml.add_entry(PropName="Energizer Sag Voltage Threshold Intercept", ShortName="energizerSagVolThresholdIntercept", Addr="340",
                      Mode="ca", Value="100", Size="1", internalOnly=False,
                      Description="")

    if HW_VER == 0:
        gen_xml.add_entry(PropName="Vbat Capacitor Dissipation Time", ShortName="vbatCapDissipationTime", Addr="341", Mode="ca", Value="10", Size="1", internalOnly=False, Description="")
    else:
        gen_xml.add_entry(PropName="Vbat Capacitor Dissipation Time", ShortName="vbatCapDissipationTime", Addr="341", Mode="ca", Value="1", Size="1", internalOnly=False, Description="")
    
    gen_xml.add_entry(PropName="No of Continuous Tadiran Readings", ShortName="noOfContinuousTadiranReadings", Addr="342",
                      Mode="ca", Value="40", Size="1", internalOnly=False,
                      Description="")
    
    gen_xml.add_entry(PropName="No Transmit Mode Exit Wait Time", ShortName="noTxExitWaitTime", Addr="343",
                      Mode="ca", Value="5", Size="1", internalOnly=False,
                      Description="")
    
    gen_xml.add_entry(PropName="No of Continuous Energizer Readings Byte 0", ShortName="noOfContinuousEnergizerReadings", Addr="344",
                      Mode="ca", Value="52", Size="2", internalOnly=False,
                      Description="")
    gen_xml.add_entry(PropName="No of Continuous Energizer Readings Byte 1", ShortName="noOfContinuousEnergizerReadings", Addr="345",
                      Mode="ca", Value="0", Size="2", internalOnly=False,
                      Description="")
    
    gen_xml.add_entry(PropName="No of Battery Reads For Low Voltage Check", ShortName="numBatteryReadsForLowVoltageCheck", Addr="346",
                      Mode="ca", Value="14", Size="1", internalOnly=False,
                      Description="")
    
    gen_xml.add_entry(PropName="No of Battery Reads For Shutdown Voltage Check", ShortName="numBatteryReadsForShdnVoltageCheck", Addr="347",
                      Mode="ca", Value="6", Size="1", internalOnly=False,
                      Description="")  

    if HW_VER == 0:
	    gen_xml.add_entry(PropName="Soft Start Phase 0 State1 ADC Threshold Byte 0",    ShortName="SoftStartPhase0State1Thresh",          Addr="348", Mode="ca", Value="8",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 0 and 1 (in phase0).")
	    gen_xml.add_entry(PropName="Soft Start Phase 0 State1 ADC Threshold Byte 1",    ShortName="SoftStartPhase0State1Thresh",          Addr="349", Mode="ca", Value="7",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 0 and 1 (in phase0).")

	    gen_xml.add_entry(PropName="Soft Start Phase 0 State2 ADC Threshold Byte 0",    ShortName="SoftStartPhase0State2Thresh",          Addr="350", Mode="ca", Value="152", Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 1 and 2 (in phase0).")
	    gen_xml.add_entry(PropName="Soft Start Phase 0 State2 ADC Threshold Byte 1",    ShortName="SoftStartPhase0State2Thresh",          Addr="351", Mode="ca", Value="8",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 1 and 2 (in phase0).")

	    gen_xml.add_entry(PropName="Soft Start Phase 0 State0 Settings",                ShortName="SoftStartPhase0State0Settings",        Addr="352", Mode="ca", Value="32",  Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 0, state 0.")

	    gen_xml.add_entry(PropName="Soft Start Phase 0 State1 Settings",                ShortName="SoftStartPhase0State1Settings",        Addr="353", Mode="ca", Value="4",   Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 0, state 1.")

	    gen_xml.add_entry(PropName="Soft Start Phase 0 State2 Settings",                ShortName="SoftStartPhase0State2Settings",        Addr="354", Mode="ca", Value="12",  Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 0, state 2.")        

	    gen_xml.add_entry(PropName="Soft Start Phase 0 Ticks Before Advancing",         ShortName="SoftStartPhase0TicksBeforeAdvancing",  Addr="355", Mode="ca", Value="30",  Size="1", internalOnly=False, Description="How long in state2 of phase 0 before advancing to next phase. Units of 10 32.768kHz ticks per increment.")

	    gen_xml.add_entry(PropName="Soft Start Phase 1 State1 ADC Threshold Byte 0",    ShortName="SoftStartPhase1State1Thresh",          Addr="356", Mode="ca", Value="8",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 0 and 1 (in phase1).")
	    gen_xml.add_entry(PropName="Soft Start Phase 1 State1 ADC Threshold Byte 1",    ShortName="SoftStartPhase1State1Thresh",          Addr="357", Mode="ca", Value="7",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 0 and 1 (in phase1).")

	    gen_xml.add_entry(PropName="Soft Start Phase 1 State2 ADC Threshold Byte 0",    ShortName="SoftStartPhase1State2Thresh",          Addr="358", Mode="ca", Value="40",  Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 1 and 2 (in phase1).")
	    gen_xml.add_entry(PropName="Soft Start Phase 1 State2 ADC Threshold Byte 1",    ShortName="SoftStartPhase1State2Thresh",          Addr="359", Mode="ca", Value="10",  Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 1 and 2 (in phase1).")

	    gen_xml.add_entry(PropName="Soft Start Phase 1 State0 Settings",                ShortName="SoftStartPhase1State0Settings",        Addr="360", Mode="ca", Value="32",  Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 1, state 0.")

	    gen_xml.add_entry(PropName="Soft Start Phase 1 State1 Settings",                ShortName="SoftStartPhase1State1Settings",        Addr="361", Mode="ca", Value="6",   Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 1, state 1.")

	    gen_xml.add_entry(PropName="Soft Start Phase 1 State2 Settings",                ShortName="SoftStartPhase1State2Settings",        Addr="362", Mode="ca", Value="7",   Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 1, state 2.")        

	    gen_xml.add_entry(PropName="Soft Start Phase 1 Ticks Before Advancing",         ShortName="SoftStartPhase1TicksBeforeAdvancing",  Addr="363", Mode="ca", Value="3",   Size="1", internalOnly=False, Description="How long in state2 of phase 1 before advancing to next phase. Units of 10 32.768kHz ticks per increment.")

	    gen_xml.add_entry(PropName="Soft Start Phase 2 State1 ADC Threshold Byte 0",    ShortName="SoftStartPhase2State1Thresh",          Addr="364", Mode="ca", Value="8",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 0 and 1 (in phase2).")
	    gen_xml.add_entry(PropName="Soft Start Phase 2 State1 ADC Threshold Byte 1",    ShortName="SoftStartPhase2State1Thresh",          Addr="365", Mode="ca", Value="7",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 0 and 1 (in phase2).")

	    gen_xml.add_entry(PropName="Soft Start Phase 2 State2 ADC Threshold Byte 0",    ShortName="SoftStartPhase2State2Thresh",          Addr="366", Mode="ca", Value="196", Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 1 and 2 (in phase2).")
	    gen_xml.add_entry(PropName="Soft Start Phase 2 State2 ADC Threshold Byte 1",    ShortName="SoftStartPhase2State2Thresh",          Addr="367", Mode="ca", Value="9",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 1 and 2 (in phase2).")

	    gen_xml.add_entry(PropName="Soft Start Phase 2 State0 Settings",                ShortName="SoftStartPhase2State0Settings",        Addr="368", Mode="ca", Value="32",  Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 2, state 0.")

	    gen_xml.add_entry(PropName="Soft Start Phase 2 State1 Settings",                ShortName="SoftStartPhase2State1Settings",        Addr="369", Mode="ca", Value="7",   Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 2, state 1.")

	    gen_xml.add_entry(PropName="Soft Start Phase 2 State2 Settings",                ShortName="SoftStartPhase2State2Settings",        Addr="370", Mode="ca", Value="15",  Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 2, state 2.")        

	    gen_xml.add_entry(PropName="Soft Start Phase 2 Ticks Before Advancing",         ShortName="SoftStartPhase2TicksBeforeAdvancing",  Addr="371", Mode="ca", Value="75",  Size="1", internalOnly=False, Description="How long in state2 of phase 2 before advancing to next phase. Units of 10 32.768kHz ticks per increment.")

	    gen_xml.add_entry(PropName="Soft Start Max Ticks Before Bailout Byte 0",        ShortName="PortInitMaxTicks",                     Addr="372", Mode="ca", Value="0",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, failure will occur if the port isn't successfully initialized in this many ticks.")
	    gen_xml.add_entry(PropName="Soft Start Max Ticks Before Bailout Byte 1",        ShortName="PortInitMaxTicks",                     Addr="373", Mode="ca", Value="64",  Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, failure will occur if the port isn't successfully initialized in this many ticks.")

    else:
	    gen_xml.add_entry(PropName="Soft Start Phase 0 State1 ADC Threshold Byte 0",    ShortName="SoftStartPhase0State1Thresh",          Addr="348", Mode="ca", Value="8",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 0 and 1 (in phase0).")
	    gen_xml.add_entry(PropName="Soft Start Phase 0 State1 ADC Threshold Byte 1",    ShortName="SoftStartPhase0State1Thresh",          Addr="349", Mode="ca", Value="7",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 0 and 1 (in phase0).")

	    gen_xml.add_entry(PropName="Soft Start Phase 0 State2 ADC Threshold Byte 0",    ShortName="SoftStartPhase0State2Thresh",          Addr="350", Mode="ca", Value="40",  Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 1 and 2 (in phase0).")
	    gen_xml.add_entry(PropName="Soft Start Phase 0 State2 ADC Threshold Byte 1",    ShortName="SoftStartPhase0State2Thresh",          Addr="351", Mode="ca", Value="10",  Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 1 and 2 (in phase0).")

	    gen_xml.add_entry(PropName="Soft Start Phase 0 State0 Settings",                ShortName="SoftStartPhase0State0Settings",        Addr="352", Mode="ca", Value="32",  Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 0, state 0.")

	    gen_xml.add_entry(PropName="Soft Start Phase 0 State1 Settings",                ShortName="SoftStartPhase0State1Settings",        Addr="353", Mode="ca", Value="4",   Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 0, state 1.")

	    gen_xml.add_entry(PropName="Soft Start Phase 0 State2 Settings",                ShortName="SoftStartPhase0State2Settings",        Addr="354", Mode="ca", Value="12",  Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 0, state 2.")        

	    gen_xml.add_entry(PropName="Soft Start Phase 0 Ticks Before Advancing",         ShortName="SoftStartPhase0TicksBeforeAdvancing",  Addr="355", Mode="ca", Value="5",   Size="1", internalOnly=False, Description="How long in state2 of phase 0 before advancing to next phase. Units of 10 32.768kHz ticks per increment.")

	    gen_xml.add_entry(PropName="Soft Start Phase 1 State1 ADC Threshold Byte 0",    ShortName="SoftStartPhase1State1Thresh",          Addr="356", Mode="ca", Value="8",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 0 and 1 (in phase1).")
	    gen_xml.add_entry(PropName="Soft Start Phase 1 State1 ADC Threshold Byte 1",    ShortName="SoftStartPhase1State1Thresh",          Addr="357", Mode="ca", Value="7",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 0 and 1 (in phase1).")

	    gen_xml.add_entry(PropName="Soft Start Phase 1 State2 ADC Threshold Byte 0",    ShortName="SoftStartPhase1State2Thresh",          Addr="358", Mode="ca", Value="40",  Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 1 and 2 (in phase1).")
	    gen_xml.add_entry(PropName="Soft Start Phase 1 State2 ADC Threshold Byte 1",    ShortName="SoftStartPhase1State2Thresh",          Addr="359", Mode="ca", Value="10",  Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 1 and 2 (in phase1).")

	    gen_xml.add_entry(PropName="Soft Start Phase 1 State0 Settings",                ShortName="SoftStartPhase1State0Settings",        Addr="360", Mode="ca", Value="32",  Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 1, state 0.")

	    gen_xml.add_entry(PropName="Soft Start Phase 1 State1 Settings",                ShortName="SoftStartPhase1State1Settings",        Addr="361", Mode="ca", Value="14",  Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 1, state 1.")

	    gen_xml.add_entry(PropName="Soft Start Phase 1 State2 Settings",                ShortName="SoftStartPhase1State2Settings",        Addr="362", Mode="ca", Value="15",  Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 1, state 2.")        

	    gen_xml.add_entry(PropName="Soft Start Phase 1 Ticks Before Advancing",         ShortName="SoftStartPhase1TicksBeforeAdvancing",  Addr="363", Mode="ca", Value="5",   Size="1", internalOnly=False, Description="How long in state2 of phase 1 before advancing to next phase. Units of 10 32.768kHz ticks per increment.")

	    gen_xml.add_entry(PropName="Soft Start Phase 2 State1 ADC Threshold Byte 0",    ShortName="SoftStartPhase2State1Thresh",          Addr="364", Mode="ca", Value="8",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 0 and 1 (in phase2).")
	    gen_xml.add_entry(PropName="Soft Start Phase 2 State1 ADC Threshold Byte 1",    ShortName="SoftStartPhase2State1Thresh",          Addr="365", Mode="ca", Value="7",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 0 and 1 (in phase2).")

	    gen_xml.add_entry(PropName="Soft Start Phase 2 State2 ADC Threshold Byte 0",    ShortName="SoftStartPhase2State2Thresh",          Addr="366", Mode="ca", Value="40",  Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 1 and 2 (in phase2).")
	    gen_xml.add_entry(PropName="Soft Start Phase 2 State2 ADC Threshold Byte 1",    ShortName="SoftStartPhase2State2Thresh",          Addr="367", Mode="ca", Value="10",  Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, this is the ADC threshold between states 1 and 2 (in phase2).")

	    gen_xml.add_entry(PropName="Soft Start Phase 2 State0 Settings",                ShortName="SoftStartPhase2State0Settings",        Addr="368", Mode="ca", Value="32",  Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 2, state 0.")

	    gen_xml.add_entry(PropName="Soft Start Phase 2 State1 Settings",                ShortName="SoftStartPhase2State1Settings",        Addr="369", Mode="ca", Value="14",   Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 2, state 1.")

	    gen_xml.add_entry(PropName="Soft Start Phase 2 State2 Settings",                ShortName="SoftStartPhase2State2Settings",        Addr="370", Mode="ca", Value="15",  Size="1", internalOnly=False, Description="When soft starting the 5V0 converter, these are the settings for phase 2, state 2.")        

	    gen_xml.add_entry(PropName="Soft Start Phase 2 Ticks Before Advancing",         ShortName="SoftStartPhase2TicksBeforeAdvancing",  Addr="371", Mode="ca", Value="5",  Size="1", internalOnly=False, Description="How long in state2 of phase 2 before advancing to next phase. Units of 10 32.768kHz ticks per increment.")

	    gen_xml.add_entry(PropName="Soft Start Max Ticks Before Bailout Byte 0",        ShortName="PortInitMaxTicks",                     Addr="372", Mode="ca", Value="0",   Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, failure will occur if the port isn't successfully initialized in this many ticks.")
	    gen_xml.add_entry(PropName="Soft Start Max Ticks Before Bailout Byte 1",        ShortName="PortInitMaxTicks",                     Addr="373", Mode="ca", Value="64",  Size="2", internalOnly=False, Description="When soft starting the 5V0 converter, failure will occur if the port isn't successfully initialized in this many ticks.")

    gen_xml.add_entry(PropName="VSWR clear Threshold byte 0", ShortName="VswrClearThreshold", Addr="482",
                      Mode="ca", Value="172", Size="2", internalOnly=False,
                      Description="")
    gen_xml.add_entry(PropName="VSWR clear Threshold byte 1", ShortName="VswrClearThreshold", Addr="483",
                      Mode="ca", Value="13", Size="2", internalOnly=False,
                      Description="")
    
    gen_xml.add_entry(PropName="Last VSWR Detect Reading Byte 0", ShortName="LastVSWRDetectReading", Addr="484",
                      Mode="ca", Value="0", Size="2", internalOnly=False,
                      Description="This location is populated whenever a VSWR detect reading is taken. "
                                  "It is a calculated ratio between VSWR_REF and RF_VREF in increments of 1 : 0.01")
    gen_xml.add_entry(PropName="Last VSWR Detect Reading Byte 1", ShortName="LastVSWRDetectReading", Addr="485",
                      Mode="ca", Value="0", Size="2", internalOnly=False,
                      Description="This location is populated whenever a VSWR detect reading is taken. "
                                  "It is a calculated ratio between VSWR_REF and RF_VREF in increments of 1 : 0.01")

    gen_xml.add_entry(PropName="VSWR Alarm Threshold Byte 0", ShortName="VSWRAlarmThreshold", Addr="486",
                      Mode="ca", Value="136", Size="2", internalOnly=False,
                      Description="VSWR level which is the fault threshold. VSWR readings above this threshold will "
                                  "trigger an alarm which will be cleared if the VSWR changes to be below this level.")
    gen_xml.add_entry(PropName="VSWR Alarm Threshold Byte 1", ShortName="VSWRAlarmThreshold", Addr="487",
                      Mode="ca", Value="19", Size="2", internalOnly=False,
                      Description="VSWR level which is the fault threshold. VSWR readings above this threshold will "
                                  "trigger an alarm which will be cleared if the VSWR changes to be below this level.")

    gen_xml.add_entry(PropName="Memory Map Configuration CRC Byte 0", ShortName="memoryMapCfgCrc", Addr="488",
                      Mode="no", Value="0", Size="2", internalOnly=False,
                      Description="This location is populated whenever a Memory Map Change Detect CRC calculation is taken.")
    gen_xml.add_entry(PropName="Memory Map Configuration CRC Byte 1", ShortName="memoryMapCfgCrc", Addr="489",
                      Mode="no", Value="0", Size="2", internalOnly=False,
                      Description="This location is populated whenever a Memory Map Change Detect CRC calculation is taken.")

    gen_xml.add_entry(PropName="Expected Program CRC Byte 0", ShortName="expectedProgramCrc", Addr="490",
                      Mode="va", Value=str(int(crcLsb, 16)), Size="2", internalOnly=False,
                      Description="This location is populated whenever a Memory Map Change Detect CRC calculation is taken "
                                  "for an expected reason.")
    gen_xml.add_entry(PropName="Expected Program CRC Byte 1", ShortName="expectedProgramCrc", Addr="491",
                      Mode="va", Value=str(int(crcMsb, 16)), Size="2", internalOnly=False,
                      Description="This location is populated whenever a Memory Map Change Detect CRC calculation is taken "
                                  "for an expected reason.")
    
    gen_xml.add_entry(PropName="Battery Statistics Message Interval", ShortName="batteryStatisticsMsgInterval",
                      Addr="495",
                      Mode="ca", Value="1", Size="1", internalOnly=False,
                      Description="Battery statistic reading taken every N times a TX occurs. Used in the 0x57 statistics message. "
                                  "Zero means do not sample.")

    gen_xml.add_entry(PropName="Temperature Statistics Message Interval",
                      ShortName="temperatureStatisticsMsgInterval",
                      Addr="496", Mode="ca", Value="0", Size="1", internalOnly=False,
                      Description="Temperature statistic reading taken every N times a TX occurs. Used in the 0x57 statistics "
                                  "message. Zero means do not sample.")

    gen_xml.add_entry(PropName="Adv. Alarms Last Read P1 Byte 0", ShortName="P1_AdvancedMeterAlarmsByte0", Addr="497",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="Last advanced alarms byte read P1 Byte0")

    gen_xml.add_entry(PropName="Adv. Alarms Last Read P1 Byte 1", ShortName="P1_AdvancedMeterAlarmsByte1", Addr="498",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="Last advanced alarms byte read P1 Byte1")

    gen_xml.add_entry(PropName="Adv. Alarms Last Read P2 Byte 0", ShortName="P2_AdvancedMeterAlarmsByte0", Addr="499",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="Last advanced alarms byte read P2 Byte0")

    gen_xml.add_entry(PropName="Adv. Alarms Last Read P2 Byte 1", ShortName="P2_AdvancedMeterAlarmsByte1", Addr="500",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="Last advanced alarms byte read P2 Byte1")
    
    gen_xml.add_entry(PropName="Adv. Alarms Last Read P1 Byte 2", ShortName="P1_AdvancedMeterAlarmsByte2", Addr="501",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="Last advanced alarms byte read P1 Byte2")

    gen_xml.add_entry(PropName="Adv. Alarms Last Read P1 Byte 3", ShortName="P1_AdvancedMeterAlarmsByte3", Addr="502",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="Last advanced alarms byte read P1 Byte3")

    gen_xml.add_entry(PropName="Adv. Alarms Last Read P2 Byte 2", ShortName="P2_AdvancedMeterAlarmsByte2", Addr="503",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="Last advanced alarms byte read P2 Byte2")

    gen_xml.add_entry(PropName="Adv. Alarms Last Read P2 Byte 3", ShortName="P2_AdvancedMeterAlarmsByte3", Addr="504",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="Last advanced alarms byte read P2 Byte3")
    
    gen_xml.add_entry(PropName="Adv. Alarms Port Settings", ShortName="advancedAlarmsPortSettings", Addr="505",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="Contains various advanced alarm settings for both ports.")
    
    gen_xml.add_entry(PropName="Bit Masks for advanced alarms P1 Byte 0", ShortName="P1_AdvancedMeterAlarmsByte0_MASK", Addr="506",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="BITWISE MASK SETTINGS for advanced alarms P1 Byte0")

    gen_xml.add_entry(PropName="Bit Masks for advanced alarms P1 Byte 1", ShortName="P1_AdvancedMeterAlarmsByte1_MASK", Addr="507",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="BITWISE MASK SETTINGS for advanced alarms P1 Byte1")

    gen_xml.add_entry(PropName="Bit Masks for advanced alarms P2 Byte 0", ShortName="P2_AdvancedMeterAlarmsByte0_MASK", Addr="508",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="BITWISE MASK SETTINGS for advanced alarms P2 Byte0")

    gen_xml.add_entry(PropName="Bit Masks for advanced alarms P2 Byte 1", ShortName="P2_AdvancedMeterAlarmsByte1_MASK", Addr="509",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="BITWISE MASK SETTINGS for advanced alarms P2 Byte1")
    
    gen_xml.add_entry(PropName="Bit Masks for advanced alarms P1 Byte 2", ShortName="P1_AdvancedMeterAlarmsByte2_MASK", Addr="510",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="BITWISE MASK SETTINGS for advanced alarms P1 Byte2")

    gen_xml.add_entry(PropName="Bit Masks for advanced alarms P1 Byte 3", ShortName="P1_AdvancedMeterAlarmsByte3_MASK", Addr="511",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="BITWISE MASK SETTINGS for advanced alarms P1 Byte3")

    gen_xml.add_entry(PropName="Bit Masks for advanced alarms P2 Byte 2", ShortName="P2_AdvancedMeterAlarmsByte2_MASK", Addr="512",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="BITWISE MASK SETTINGS for advanced alarms P2 Byte2")

    gen_xml.add_entry(PropName="Bit Masks for advanced alarms P2 Byte 3", ShortName="P2_AdvancedMeterAlarmsByte3_MASK", Addr="513",
                      Mode="no", Value="0", Size="1", internalOnly=False,
                      Description="BITWISE MASK SETTINGS for advanced alarms P2 Byte3")

    gen_xml.add_entry(PropName="Moisture Alarm Threshold (LSB)", ShortName="moistureDetectAlarmThreshold",
                      Addr="576", Mode="ca", Value="72", Size="2", internalOnly=False,
                      Description="The value at which a moisture detection alarm will be sent (if enabled).  (10,000 corresponds to 100% relative humidity)")
    gen_xml.add_entry(PropName="Moisture Alarm Threshold (MSB)", ShortName="moistureDetectAlarmThreshold",
                      Addr="577", Mode="ca", Value="38", Size="2", internalOnly=False,
                      Description="The value at which a moisture detection alarm will be sent (if enabled).  (10,000 corresponds to 100% relative humidity)")

    gen_xml.add_entry(PropName="Last Alarm Status Transmitted (LSB)", ShortName="LastAlarmStatusTransmitted",
                      Addr="578", Mode="ca", Value="0", Size="2", internalOnly=False,
                      Description="The last value of AlarmStatus transmitted in an alarm message")
    gen_xml.add_entry(PropName="Last Alarm Status Transmitted (MSB)", ShortName="LastAlarmStatusTransmitted",
                      Addr="579", Mode="ca", Value="0", Size="2", internalOnly=False,
                      Description="The last value of AlarmStatus transmitted in an alarm message")

    gen_xml.add_entry(PropName="Alarm Blackout Time[0] (LSB)", ShortName="AlarmBlackout0",   Addr="580", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Out of Memory Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[0] (MSB)", ShortName="AlarmBlackout0",   Addr="581", Mode="ca", Value="90", Size="2", internalOnly=False, Description="Out of Memory Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[1] (LSB)", ShortName="AlarmBlackout1",   Addr="582", Mode="ca", Value="96", Size="2", internalOnly=False, Description="Moisture Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[1] (MSB)", ShortName="AlarmBlackout1",   Addr="583", Mode="ca", Value="39", Size="2", internalOnly=False, Description="Moisture Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[2] (LSB)", ShortName="AlarmBlackout2",   Addr="584", Mode="ca", Value="1", Size="2", internalOnly=False, Description="Program Memory Error Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[2] (MSB)", ShortName="AlarmBlackout2",   Addr="585", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Program Memory Error Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[3] (LSB)", ShortName="AlarmBlackout3",   Addr="586", Mode="ca", Value="1", Size="2", internalOnly=False, Description="Memory Map Error Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[3] (MSB)", ShortName="AlarmBlackout3",   Addr="587", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Memory Map Error Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[4] (LSB)", ShortName="AlarmBlackout4",   Addr="588", Mode="ca", Value="1", Size="2", internalOnly=False, Description="Energizer Last Gasp Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[4] (MSB)", ShortName="AlarmBlackout4",   Addr="589", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Energizer Last Gasp Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[5] (LSB)", ShortName="AlarmBlackout5",   Addr="590", Mode="ca", Value="1", Size="2", internalOnly=False, Description="Port1 Cut Wire Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[5] (MSB)", ShortName="AlarmBlackout5",   Addr="591", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Port1 Cut Wire Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[6] (LSB)", ShortName="AlarmBlackout6",   Addr="592", Mode="ca", Value="1", Size="2", internalOnly=False, Description="Serial Comm Problem Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[6] (MSB)", ShortName="AlarmBlackout6",   Addr="593", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Serial Comm Problem Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[7] (LSB)", ShortName="AlarmBlackout7",   Addr="594", Mode="ca", Value="1", Size="2", internalOnly=False, Description="Tadiran Last Gasp Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[7] (MSB)", ShortName="AlarmBlackout7",   Addr="595", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Tadiran Last Gasp Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[8] (LSB)", ShortName="AlarmBlackout8",   Addr="596", Mode="ca", Value="1", Size="2", internalOnly=False, Description="Tilt Tamper Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[8] (MSB)", ShortName="AlarmBlackout8",   Addr="597", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Tilt Tamper Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[9] (LSB)", ShortName="AlarmBlackout9",   Addr="598", Mode="ca", Value="1", Size="2", internalOnly=False, Description="Mag Tamper Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[9] (MSB)", ShortName="AlarmBlackout9",   Addr="599", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Mag Tamper Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[10] (LSB)", ShortName="AlarmBlackout10", Addr="600", Mode="ca", Value="96", Size="2", internalOnly=False, Description="Bad VSWR Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[10] (MSB)", ShortName="AlarmBlackout10", Addr="601", Mode="ca", Value="39", Size="2", internalOnly=False, Description="Bad VSWR Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[11] (LSB)", ShortName="AlarmBlackout11", Addr="602", Mode="ca", Value="60", Size="2", internalOnly=False, Description="Port2 Cut Wire Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[11] (MSB)", ShortName="AlarmBlackout11", Addr="603", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Port2 Cut Wire Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[12] (LSB)", ShortName="AlarmBlackout12", Addr="604", Mode="ca", Value="60", Size="2", internalOnly=False, Description="PCI Tamper Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[12] (MSB)", ShortName="AlarmBlackout12", Addr="605", Mode="ca", Value="0", Size="2", internalOnly=False, Description="PCI Tamper Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[13] (LSB)", ShortName="AlarmBlackout13", Addr="606", Mode="ca", Value="1", Size="2", internalOnly=False, Description="Cover Tamper Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[13] (MSB)", ShortName="AlarmBlackout13", Addr="607", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Cover Tamper Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[14] (LSB)", ShortName="AlarmBlackout14", Addr="608", Mode="ca", Value="60", Size="2", internalOnly=False, Description="Reverse Flow Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[14] (MSB)", ShortName="AlarmBlackout14", Addr="609", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Reverse Flow Alarm Blackout Time (s)")      

    gen_xml.add_entry(PropName="Alarm Blackout Time[15] (LSB)", ShortName="AlarmBlackout15", Addr="610", Mode="ca", Value="60", Size="2", internalOnly=False, Description="Cut Serial Cable Alarm Blackout Time (s)")
    gen_xml.add_entry(PropName="Alarm Blackout Time[15] (MSB)", ShortName="AlarmBlackout15", Addr="611", Mode="ca", Value="0", Size="2", internalOnly=False, Description="Cut Serial Cable Alarm Blackout Time (s)")      

    if hasRDD:
        gen_xml.add_entry(PropName="RDD Status", ShortName="RddStatus", Addr="612", Mode="no", Value="0",
                          Size="1", internalOnly=False,
                          Description="Status of the RDD (0 disabled, 1 busy, 2 error on last op, 3 idle)")

        gen_xml.add_entry(PropName="RDD Valve Position", ShortName="RddPosition", Addr="613", Mode="no",
                          Value="0", Size="1", internalOnly=False,
                          Description="Position of the RDD (0 unknown, 1 closed, 2 open, 3 partially open, 4 in transit)")

        gen_xml.add_entry(PropName="RDD Battery Status", ShortName="RddBatteryStatus", Addr="614", Mode="no",
                          Value="255", Size="1", internalOnly=False,
                          Description="RDD Battery Status (0 low, 1 ok, 0xff unknown)")

        gen_xml.add_entry(PropName="RDD Previous Command", ShortName="RddPrevCmd", Addr="615", Mode="no", Value="255",
                          Size="1", internalOnly=False,
                          Description="Last RDD Command (1 open, 2 close, 3 open halfway, 4 do sediment turn, 0xff unknown)")

        gen_xml.add_entry(PropName="RDD Previous Command Status", ShortName="RddPrevCmdStatus", Addr="616", Mode="no",
                          Value="255", Size="1", internalOnly=False,
                          Description="Previous RDD Command Status (0 failed, 1 succeeded, 0xff unknown)")

        gen_xml.add_entry(PropName="RDD Serial Number[1]", ShortName="rddSerialNumber", Addr="617", Mode="no",
                          Value="0", Size="9", internalOnly=False, Description="RDD Serial Number")
        gen_xml.add_entry(PropName="RDD Serial Number[2]", ShortName="rddSerialNumber", Addr="618", Mode="no",
                          Value="0", Size="9", internalOnly=False, Description="RDD Serial Number")
        gen_xml.add_entry(PropName="RDD Serial Number[3]", ShortName="rddSerialNumber", Addr="619", Mode="no",
                          Value="0", Size="9", internalOnly=False, Description="RDD Serial Number")
        gen_xml.add_entry(PropName="RDD Serial Number[4]", ShortName="rddSerialNumber", Addr="620", Mode="no",
                          Value="0", Size="9", internalOnly=False, Description="RDD Serial Number")
        gen_xml.add_entry(PropName="RDD Serial Number[5]", ShortName="rddSerialNumber", Addr="621", Mode="no",
                          Value="0", Size="9", internalOnly=False, Description="RDD Serial Number")
        gen_xml.add_entry(PropName="RDD Serial Number[6]", ShortName="rddSerialNumber", Addr="622", Mode="no",
                          Value="0", Size="9", internalOnly=False, Description="RDD Serial Number")
        gen_xml.add_entry(PropName="RDD Serial Number[7]", ShortName="rddSerialNumber", Addr="623", Mode="no",
                          Value="0", Size="9", internalOnly=False, Description="RDD Serial Number")
        gen_xml.add_entry(PropName="RDD Serial Number[8]", ShortName="rddSerialNumber", Addr="624", Mode="no",
                          Value="0", Size="9", internalOnly=False, Description="RDD Serial Number")
        gen_xml.add_entry(PropName="RDD Serial Number[9]", ShortName="rddSerialNumber", Addr="625", Mode="no",
                          Value="0", Size="9", internalOnly=False, Description="RDD Serial Number")

    gen_xml.add_entry(PropName="DCU History (1) RSSI", ShortName="DCU_1_RSSI", Addr="640", Mode="no", Value="0",
                      Size="1", internalOnly=False, Description="1st Best packet RSSI")

    gen_xml.add_entry(PropName="DCU History (1) Frequency Error", ShortName="DCU_1_FrequencyError", Addr="641",
                      Mode="no", Value="0", Size="1", internalOnly=False, Description="1st Best packet Freq Error")

    gen_xml.add_entry(PropName="DCU History (1) DCU ID (Byte 0)", ShortName="DCU_1_ID", Addr="642", Mode="no",
                      Value="0", Size="4", internalOnly=False,
                      Description="1st Best DCU/MTU ID (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (1) DCU ID (Byte 1)", ShortName="DCU_1_ID", Addr="643", Mode="no",
                      Value="0", Size="4", internalOnly=False,
                      Description="1st Best DCU/MTU ID (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (1) DCU ID (Byte 2)", ShortName="DCU_1_ID", Addr="644", Mode="no",
                      Value="0", Size="4", internalOnly=False,
                      Description="1st Best DCU/MTU ID (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (1) DCU ID (Byte 3)", ShortName="DCU_1_ID", Addr="645", Mode="no",
                      Value="0", Size="4", internalOnly=False,
                      Description="1st Best DCU/MTU ID (lsb first -- little endian)")

    gen_xml.add_entry(PropName="DCU History (1) Timestamp (Byte 0)", ShortName="DCU_1_Timestamp", Addr="646",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="1st Best Timestamp (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (1) Timestamp (Byte 1)", ShortName="DCU_1_Timestamp", Addr="647",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="1st Best Timestamp (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (1) Timestamp (Byte 2)", ShortName="DCU_1_Timestamp", Addr="648",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="1st Best Timestamp (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (1) Timestamp (Byte 3)", ShortName="DCU_1_Timestamp", Addr="649",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="1st Best Timestamp (lsb first -- little endian)")

    gen_xml.add_entry(PropName="DCU History (2) RSSI", ShortName="DCU_2_RSSI", Addr="650", Mode="no", Value="0",
                      Size="1", internalOnly=False, Description="2nd Best packet RSSI")

    gen_xml.add_entry(PropName="DCU History (2) Frequency Error", ShortName="DCU_2_FrequencyError", Addr="651",
                      Mode="no", Value="0", Size="1", internalOnly=False, Description="2nd Best packet Freq Error")

    gen_xml.add_entry(PropName="DCU History (2) DCU ID (Byte 0)", ShortName="DCU_2_ID", Addr="652", Mode="no",
                      Value="0", Size="4", internalOnly=False,
                      Description="2nd Best DCU/MTU ID (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (2) DCU ID (Byte 1)", ShortName="DCU_2_ID", Addr="653", Mode="no",
                      Value="0", Size="4", internalOnly=False,
                      Description="2nd Best DCU/MTU ID (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (2) DCU ID (Byte 2)", ShortName="DCU_2_ID", Addr="654", Mode="no",
                      Value="0", Size="4", internalOnly=False,
                      Description="2nd Best DCU/MTU ID (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (2) DCU ID (Byte 3)", ShortName="DCU_2_ID", Addr="655", Mode="no",
                      Value="0", Size="4", internalOnly=False,
                      Description="2nd Best DCU/MTU ID (lsb first -- little endian)")

    gen_xml.add_entry(PropName="DCU History (2) Timestamp (Byte 0)", ShortName="DCU_2_Timestamp", Addr="656",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="2nd Best Timestamp (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (2) Timestamp (Byte 1)", ShortName="DCU_2_Timestamp", Addr="657",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="2nd Best Timestamp (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (2) Timestamp (Byte 2)", ShortName="DCU_2_Timestamp", Addr="658",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="2nd Best Timestamp (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (2) Timestamp (Byte 3)", ShortName="DCU_2_Timestamp", Addr="659",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="2nd Best Timestamp (lsb first -- little endian)")

    gen_xml.add_entry(PropName="DCU History (3) RSSI", ShortName="DCU_3_RSSI", Addr="660", Mode="no", Value="0",
                      Size="1", internalOnly=False, Description="3rd Best packet RSSI")

    gen_xml.add_entry(PropName="DCU History (3) Frequency Error", ShortName="DCU_3_FrequencyError", Addr="661",
                      Mode="no", Value="0", Size="1", internalOnly=False, Description="3rd Best packet Freq Error")

    gen_xml.add_entry(PropName="DCU History (3) DCU ID (Byte 0)", ShortName="DCU_3_ID", Addr="662", Mode="no",
                      Value="0", Size="4", internalOnly=False,
                      Description="3rd Best DCU/MTU ID (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (3) DCU ID (Byte 1)", ShortName="DCU_3_ID", Addr="663", Mode="no",
                      Value="0", Size="4", internalOnly=False,
                      Description="3rd Best DCU/MTU ID (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (3) DCU ID (Byte 2)", ShortName="DCU_3_ID", Addr="664", Mode="no",
                      Value="0", Size="4", internalOnly=False,
                      Description="3rd Best DCU/MTU ID (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (3) DCU ID (Byte 3)", ShortName="DCU_3_ID", Addr="665", Mode="no",
                      Value="0", Size="4", internalOnly=False,
                      Description="3rd Best DCU/MTU ID (lsb first -- little endian)")

    gen_xml.add_entry(PropName="DCU History (3) Timestamp (Byte 0)", ShortName="DCU_3_Timestamp", Addr="666",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="3rd Best Timestamp (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (3) Timestamp (Byte 1)", ShortName="DCU_3_Timestamp", Addr="667",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="3rd Best Timestamp (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (3) Timestamp (Byte 2)", ShortName="DCU_3_Timestamp", Addr="668",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="3rd Best Timestamp (lsb first -- little endian)")
    gen_xml.add_entry(PropName="DCU History (3) Timestamp (Byte 3)", ShortName="DCU_3_Timestamp", Addr="669",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="3rd Best Timestamp (lsb first -- little endian)")

    gen_xml.add_entry(PropName="CRC Failure Counter 0", ShortName="CrcFailureCounter", Addr="670", Mode="ca",
                      Value="0", Size="2", internalOnly=False, Description="")
    gen_xml.add_entry(PropName="CRC Failure Counter 1", ShortName="CrcFailureCounter", Addr="671", Mode="ca",
                      Value="0", Size="2", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="FEC Corrected Packets Counter 0", ShortName="FecCorrectedCounter", Addr="672",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="")
    gen_xml.add_entry(PropName="FEC Corrected Packets Counter 1", ShortName="FecCorrectedCounter", Addr="673",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="FEC Correction Failure Counter 0", ShortName="FecFailureCounter", Addr="674",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="")
    gen_xml.add_entry(PropName="FEC Correction Failure Counter 1", ShortName="FecFailureCounter", Addr="675",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Packet RX Mismatched Address Counter 0", ShortName="MisMatchAddressCounter",
                      Addr="676", Mode="ca", Value="0", Size="2", internalOnly=False, Description="")
    gen_xml.add_entry(PropName="Packet RX Mismatched Address Counter 1", ShortName="MisMatchAddressCounter",
                      Addr="677", Mode="ca", Value="0", Size="2", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Packet RX Matching Address Counter 0", ShortName="MatchingAddressCounter",
                      Addr="678", Mode="ca", Value="0", Size="2", internalOnly=False, Description="")
    gen_xml.add_entry(PropName="Packet RX Matching Address Counter 1", ShortName="MatchingAddressCounter",
                      Addr="679", Mode="ca", Value="0", Size="2", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Packet RX Broadcast Counter 0", ShortName="BroadCastMsgCounter", Addr="680",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="")
    gen_xml.add_entry(PropName="Packet RX Broadcast Counter 1", ShortName="BroadCastMsgCounter", Addr="681",
                      Mode="ca", Value="0", Size="2", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="RF Bytes Transmitted Counter 0", ShortName="RF_TxByteCount", Addr="682",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="Number of bytes the RF transmitter has sent.")
    gen_xml.add_entry(PropName="RF Bytes Transmitted Counter 1", ShortName="RF_TxByteCount", Addr="683",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="Number of bytes the RF transmitter has sent.")
    gen_xml.add_entry(PropName="RF Bytes Transmitted Counter 2", ShortName="RF_TxByteCount", Addr="684",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="Number of bytes the RF transmitter has sent.")
    gen_xml.add_entry(PropName="RF Bytes Transmitted Counter 3", ShortName="RF_TxByteCount", Addr="685",
                      Mode="no", Value="0", Size="4", internalOnly=False,
                      Description="Number of bytes the RF transmitter has sent.")

    gen_xml.add_entry(PropName="DCU ID of Last Timesync Received 0", ShortName="DcuIDOfLastTimesyncReceived",
                      Addr="686", Mode="ca", Value="0", Size="4", internalOnly=False,
                      Description="DCU ID contained in the last timesync we received.")
    gen_xml.add_entry(PropName="DCU ID of Last Timesync Received 1", ShortName="DcuIDOfLastTimesyncReceived",
                      Addr="687", Mode="ca", Value="0", Size="4", internalOnly=False,
                      Description="DCU ID contained in the last timesync we received.")
    gen_xml.add_entry(PropName="DCU ID of Last Timesync Received 2", ShortName="DcuIDOfLastTimesyncReceived",
                      Addr="688", Mode="ca", Value="0", Size="4", internalOnly=False,
                      Description="DCU ID contained in the last timesync we received.")
    gen_xml.add_entry(PropName="DCU ID of Last Timesync Received 3", ShortName="DcuIDOfLastTimesyncReceived",
                      Addr="689", Mode="ca", Value="0", Size="4", internalOnly=False,
                      Description="DCU ID contained in the last timesync we received.")

    gen_xml.add_entry(PropName="Radio Scheduler Out of Memory Errors", ShortName="RM_OutOfMemory", Addr="690",
                      Mode="no", Value="0", Size="1", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Packet Block Manager Out of Memory Errors", ShortName="PBM_OutOfMemory",
                      Addr="691", Mode="no", Value="0", Size="1", internalOnly=False, Description="")

    gen_xml.add_entry(PropName="Fast Messaging Test Patterns Received 0", ShortName="fastMsgTestPatternReqRecvd",
                      Addr="692", Mode="no", Value="0", Size="2", internalOnly=False,
                      Description="Rollover counter, initialized to 0.")
    gen_xml.add_entry(PropName="Fast Messaging Test Patterns Received 1", ShortName="fastMsgTestPatternReqRecvd",
                      Addr="693", Mode="no", Value="0", Size="2", internalOnly=False,
                      Description="Rollover counter, initialized to 0.")

    gen_xml.add_entry(PropName="Max Star Buffers Used", ShortName="MaxStarBuffUsed", Addr="694", Mode="no",
                      Value="0", Size="1", internalOnly=False, Description="Max Star Buff Used")

    gen_xml.add_entry(PropName="RF Number of Bytes Received 0", ShortName="RF_RxByteCount", Addr="696",
                      Mode="no", Value="0", Size="4", internalOnly=True,
                      Description="Estimate of the number of RF Bytes that have been received (including sync, FEC, CRC, and preamble bytes).")
    gen_xml.add_entry(PropName="RF Number of Bytes Received 1", ShortName="RF_RxByteCount", Addr="697",
                      Mode="no", Value="0", Size="4", internalOnly=True,
                      Description="Estimate of the number of RF Bytes that have been received (including sync, FEC, CRC, and preamble bytes).")
    gen_xml.add_entry(PropName="RF Number of Bytes Received 2", ShortName="RF_RxByteCount", Addr="698",
                      Mode="no", Value="0", Size="4", internalOnly=True,
                      Description="Estimate of the number of RF Bytes that have been received (including sync, FEC, CRC, and preamble bytes).")
    gen_xml.add_entry(PropName="RF Number of Bytes Received 3", ShortName="RF_RxByteCount", Addr="699",
                      Mode="no", Value="0", Size="4", internalOnly=True,
                      Description="Estimate of the number of RF Bytes that have been received (including sync, FEC, CRC, and preamble bytes).")

    gen_xml.add_entry(PropName="Lifetime Receive Time Scheduled (ms) 0",
                      ShortName="LifetimeReceiveTimeScheduled", Addr="700", Mode="no", Value="0", Size="4",
                      internalOnly=True,
                      Description="Number of milliseconds of RF receive time that has been scheduled over the lifetime of the product.")
    gen_xml.add_entry(PropName="Lifetime Receive Time Scheduled (ms) 1",
                      ShortName="LifetimeReceiveTimeScheduled", Addr="701", Mode="no", Value="0", Size="4",
                      internalOnly=True,
                      Description="Number of milliseconds of RF receive time that has been scheduled over the lifetime of the product.")
    gen_xml.add_entry(PropName="Lifetime Receive Time Scheduled (ms) 2",
                      ShortName="LifetimeReceiveTimeScheduled", Addr="702", Mode="no", Value="0", Size="4",
                      internalOnly=True,
                      Description="Number of milliseconds of RF receive time that has been scheduled over the lifetime of the product.")
    gen_xml.add_entry(PropName="Lifetime Receive Time Scheduled (ms) 3",
                      ShortName="LifetimeReceiveTimeScheduled", Addr="703", Mode="no", Value="0", Size="4",
                      internalOnly=True,
                      Description="Number of milliseconds of RF receive time that has been scheduled over the lifetime of the product.")

    gen_xml.add_entry(PropName="System Lifetime 0", ShortName="systemLifetime", Addr="704", Mode="no", Value="0",
                      Size="4", internalOnly=True,
                      Description="Seconds the MTU has been 'alive'. Counter increments every second that the MTU is running.")
    gen_xml.add_entry(PropName="System Lifetime 1", ShortName="systemLifetime", Addr="705", Mode="no", Value="0",
                      Size="4", internalOnly=True,
                      Description="Seconds the MTU has been 'alive'. Counter increments every second that the MTU is running.")
    gen_xml.add_entry(PropName="System Lifetime 2", ShortName="systemLifetime", Addr="706", Mode="no", Value="0",
                      Size="4", internalOnly=True,
                      Description="Seconds the MTU has been 'alive'. Counter increments every second that the MTU is running.")
    gen_xml.add_entry(PropName="System Lifetime 3", ShortName="systemLifetime", Addr="707", Mode="no", Value="0",
                      Size="4", internalOnly=True,
                      Description="Seconds the MTU has been 'alive'. Counter increments every second that the MTU is running.")

    gen_xml.add_entry(PropName="Count of non-authentic packets received 0", ShortName="SecurityFailureCounter",
                      Addr="708", Mode="no", Value="0", Size="2", internalOnly=False,
                      Description="Count of packets received which failed the authenticity checks. Rollover counter, initialized to 0.")
    gen_xml.add_entry(PropName="Count of non-authentic packets received 1", ShortName="SecurityFailureCounter",
                      Addr="709", Mode="no", Value="0", Size="2", internalOnly=False,
                      Description="Count of packets received which failed the authenticity checks. Rollover counter, initialized to 0.")

    gen_xml.add_entry(PropName="Missing F/W Packet Response Dither",
                      ShortName="MissingFwPacketResponseDither", Addr="710", Mode="ca", Value="20", Size="1",
                      internalOnly=False,
                      Description="How long to dither responses to a missing f/w packet request during a FOTA. (15 seconds per increment)")

    gen_xml.add_entry(PropName="Number of bytes available in RF_LastPktRcvd",
                      ShortName="RF_NumBytesInLastPktRcvd", Addr="711", Mode="no", Value="0", Size="1",
                      internalOnly=True,
                      Description="Number of bytes in the next array.")    

    gen_xml.add_entry(PropName="RF_LastPktRcvd00", ShortName="RF_LastPktRcvd", Addr="712", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd01", ShortName="RF_LastPktRcvd", Addr="713", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd02", ShortName="RF_LastPktRcvd", Addr="714", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd03", ShortName="RF_LastPktRcvd", Addr="715", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd04", ShortName="RF_LastPktRcvd", Addr="716", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd05", ShortName="RF_LastPktRcvd", Addr="717", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd06", ShortName="RF_LastPktRcvd", Addr="718", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd07", ShortName="RF_LastPktRcvd", Addr="719", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd08", ShortName="RF_LastPktRcvd", Addr="720", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd09", ShortName="RF_LastPktRcvd", Addr="721", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd10", ShortName="RF_LastPktRcvd", Addr="722", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd11", ShortName="RF_LastPktRcvd", Addr="723", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd12", ShortName="RF_LastPktRcvd", Addr="724", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd13", ShortName="RF_LastPktRcvd", Addr="725", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd14", ShortName="RF_LastPktRcvd", Addr="726", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd15", ShortName="RF_LastPktRcvd", Addr="727", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd16", ShortName="RF_LastPktRcvd", Addr="728", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd17", ShortName="RF_LastPktRcvd", Addr="729", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd18", ShortName="RF_LastPktRcvd", Addr="730", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd19", ShortName="RF_LastPktRcvd", Addr="731", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd20", ShortName="RF_LastPktRcvd", Addr="732", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd21", ShortName="RF_LastPktRcvd", Addr="733", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd22", ShortName="RF_LastPktRcvd", Addr="734", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd23", ShortName="RF_LastPktRcvd", Addr="735", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd24", ShortName="RF_LastPktRcvd", Addr="736", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd25", ShortName="RF_LastPktRcvd", Addr="737", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd26", ShortName="RF_LastPktRcvd", Addr="738", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd27", ShortName="RF_LastPktRcvd", Addr="739", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd28", ShortName="RF_LastPktRcvd", Addr="740", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd29", ShortName="RF_LastPktRcvd", Addr="741", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd30", ShortName="RF_LastPktRcvd", Addr="742", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd31", ShortName="RF_LastPktRcvd", Addr="743", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd32", ShortName="RF_LastPktRcvd", Addr="744", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd33", ShortName="RF_LastPktRcvd", Addr="745", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd34", ShortName="RF_LastPktRcvd", Addr="746", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd35", ShortName="RF_LastPktRcvd", Addr="747", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd36", ShortName="RF_LastPktRcvd", Addr="748", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd37", ShortName="RF_LastPktRcvd", Addr="749", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd38", ShortName="RF_LastPktRcvd", Addr="750", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd39", ShortName="RF_LastPktRcvd", Addr="751", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd40", ShortName="RF_LastPktRcvd", Addr="752", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd41", ShortName="RF_LastPktRcvd", Addr="753", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd42", ShortName="RF_LastPktRcvd", Addr="754", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd43", ShortName="RF_LastPktRcvd", Addr="755", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd44", ShortName="RF_LastPktRcvd", Addr="756", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd45", ShortName="RF_LastPktRcvd", Addr="757", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd46", ShortName="RF_LastPktRcvd", Addr="758", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd47", ShortName="RF_LastPktRcvd", Addr="759", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd48", ShortName="RF_LastPktRcvd", Addr="760", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd49", ShortName="RF_LastPktRcvd", Addr="761", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd50", ShortName="RF_LastPktRcvd", Addr="762", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd51", ShortName="RF_LastPktRcvd", Addr="763", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd52", ShortName="RF_LastPktRcvd", Addr="764", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd53", ShortName="RF_LastPktRcvd", Addr="765", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd54", ShortName="RF_LastPktRcvd", Addr="766", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")
    gen_xml.add_entry(PropName="RF_LastPktRcvd55", ShortName="RF_LastPktRcvd", Addr="767", Mode="no", Value="0",
                      Size="56", internalOnly=True,
                      Description="Copy of last packet received by the low level RF code.")

    if meterType == "EXT_ALARMS":
        gen_xml.add_entry(PropName="ECODER Pattern Interval Seconds (LSB)", ShortName="ecoderPatternIntervalSeconds", Addr="770", Mode="no",
                          Value="132", Size="2", internalOnly=True, Description="")
        gen_xml.add_entry(PropName="ECODER Pattern Interval Seconds (MSB)", ShortName="ecoderPatternIntervalSeconds", Addr="771", Mode="no",
                          Value="3", Size="2", internalOnly=True, Description="")

    gen_xml.add_entry(PropName="File ID", ShortName="fileID", Addr="772", Mode="no", Value="0", Size="1",
                      internalOnly=True, Description="")

    gen_xml.add_entry(PropName="RF TX Lockup Count", ShortName="RfTxLockupCount", Addr="773", Mode="no",
                      Value="0", Size="1", internalOnly=True, Description="")

    gen_xml.add_entry(PropName="LFXT Fault Counter LSB", ShortName="LfxtOscFaultCt", Addr="774", Mode="no",
                      Value="0", Size="2", internalOnly=True, Description="")
    gen_xml.add_entry(PropName="LFXT Fault Counter MSB", ShortName="LfxtOscFaultCt", Addr="775", Mode="no",
                      Value="0", Size="2", internalOnly=True, Description="")

    gen_xml.add_entry(PropName="HFXT Fault Counter LSB", ShortName="HfxtOscFaultCt", Addr="776", Mode="no",
                      Value="0", Size="2", internalOnly=True, Description="")
    gen_xml.add_entry(PropName="HFXT Fault Counter MSB", ShortName="HfxtOscFaultCt", Addr="777", Mode="no",
                      Value="0", Size="2", internalOnly=True, Description="")

    gen_xml.add_entry(PropName="Last Time Sync Adjustment[0]", ShortName="lastTimeSyncAdjustment", Addr="780",
                      Mode="no", Value="0", Size="4", internalOnly=True, Description="")
    gen_xml.add_entry(PropName="Last Time Sync Adjustment[1]", ShortName="lastTimeSyncAdjustment", Addr="781",
                      Mode="no", Value="0", Size="4", internalOnly=True, Description="")
    gen_xml.add_entry(PropName="Last Time Sync Adjustment[2]", ShortName="lastTimeSyncAdjustment", Addr="782",
                      Mode="no", Value="0", Size="4", internalOnly=True, Description="")
    gen_xml.add_entry(PropName="Last Time Sync Adjustment[3]", ShortName="lastTimeSyncAdjustment", Addr="783",
                      Mode="no", Value="0", Size="4", internalOnly=True, Description="")

    gen_xml.add_entry(PropName="Tertiary Response Repeat Delay (LSB)", ShortName="tertiaryResponseRepeatDelayMs",
                      Addr="784", Mode="no", Value="150", Size="2", internalOnly=True,
                      Description="Time between the start of successive repeats of a tertiary reponse message")
    gen_xml.add_entry(PropName="Tertiary Response Repeat Delay (MSB)", ShortName="tertiaryResponseRepeatDelayMs",
                      Addr="785", Mode="no", Value="0", Size="2", internalOnly=True,
                      Description="Time between the start of successive repeats of a tertiary reponse message")

    gen_xml.add_entry(PropName="Star Packet Block Free Count", ShortName="StarPktBlockFreeCount", Addr="786",
                      Mode="no", Value="0", Size="1", internalOnly=True,
                      Description="Keep track of how many star packet blocks are free")

    gen_xml.add_entry(PropName="Memory Verification Interval (LSB)", ShortName="MemCheckIntervalInMinutes",
                      Addr="788",
                      Mode="no", Value="160", Size="2", internalOnly=True,
                      Description="How long between periodic memory checks (program and memMap) in minutes.")
    gen_xml.add_entry(PropName="Memory Verification Interval", ShortName="MemCheckIntervalInMinutes", Addr="789",
                      Mode="no", Value="5", Size="2", internalOnly=True,
                      Description="How long between periodic memory checks (program and memMap) in minutes.")

    gen_xml.add_entry(PropName="Si446x Debug Options (LSB)", ShortName="Si446xDebugOptions", Addr="798",
                      Mode="no", Value="0", Size="2", internalOnly=True,
                      Description="Debug options for the Si446x_driver module")
    gen_xml.add_entry(PropName="Si446x Debug Options (MSB)", ShortName="Si446xDebugOptions", Addr="799",
                      Mode="no", Value="0", Size="2", internalOnly=True,
                      Description="Debug options for the Si446x_driver module")

    gen_xml.add_entry(PropName="Free Event Low Water Mark", ShortName="FreeEventPoolLowWaterMark", Addr="804",
                      Mode="no", Value="0", Size="1", internalOnly=True, Description="")

    gen_xml.add_entry(PropName="Watchdog Timeout State Debug Variables[0]", ShortName="watchdogTimeoutState",
                      Addr="805", Mode="no", Value="0", Size="4", internalOnly=True,
                      Description="watchdog timeout state information (debug root cause of timeout)")
    gen_xml.add_entry(PropName="Watchdog Timeout State Debug Variables[1]", ShortName="watchdogTimeoutState",
                      Addr="806", Mode="no", Value="0", Size="4", internalOnly=True,
                      Description="watchdog timeout state information (debug root cause of timeout)")
    gen_xml.add_entry(PropName="Watchdog Timeout State Debug Variables[2]", ShortName="watchdogTimeoutState",
                      Addr="807", Mode="no", Value="0", Size="4", internalOnly=True,
                      Description="watchdog timeout state information (debug root cause of timeout)")
    gen_xml.add_entry(PropName="Watchdog Timeout State Debug Variables[3]", ShortName="watchdogTimeoutState",
                      Addr="808", Mode="no", Value="0", Size="4", internalOnly=True,
                      Description="watchdog timeout state information (debug root cause of timeout)")

    gen_xml.add_entry(PropName="MTU State", ShortName="MTUState", Addr="809", Mode="no", Value="0", Size="1",
                      internalOnly=True, Description="")

    gen_xml.add_entry(PropName="Max random delta byte 0", ShortName="maxRandomDelta", Addr="812", Mode="no", Value="0", Size="2",
                      internalOnly=True, Description="")
    gen_xml.add_entry(PropName="Max random delta byte 1", ShortName="maxRandomDelta", Addr="813", Mode="no", Value="0", Size="2",
                      internalOnly=True, Description="")

    gen_xml.add_entry(PropName="Min random delta byte 0", ShortName="minRandomDelta", Addr="814", Mode="no", Value="0", Size="2",
                      internalOnly=True, Description="")
    gen_xml.add_entry(PropName="Min random delta byte 1", ShortName="minRandomDelta", Addr="815", Mode="no", Value="0", Size="2",
                      internalOnly=True, Description="")

    gen_xml.add_entry(PropName="VSWR datalog entry delta change (byte 0)", ShortName="VSWR_deltaThreshold", Addr="818",
                      Mode="no", Value="232", Size="2", internalOnly=True,
                      Description="The magnitude of VSWR change that must occur before a datalog entry is created.")
    gen_xml.add_entry(PropName="VSWR datalog entry delta change (byte 1)", ShortName="VSWR_deltaThreshold", Addr="819",
                      Mode="no", Value="3", Size="2", internalOnly=True,
                      Description="The magnitude of VSWR change that must occur before a datalog entry is created.")

    gen_xml.add_entry(PropName="Tadiran average", ShortName="tadiranAvg", Addr="820", Mode="no", Value="0", Size="1",
                      internalOnly=False, Description="Tadiran battery health message scaled average")

    gen_xml.add_entry(PropName="Energizer average", ShortName="energizerAvg", Addr="821", Mode="no", Value="0", Size="1",
                      internalOnly=False, Description="Energizer battery health message scaled average")

    gen_xml.add_entry(PropName="Battery voltage measurement task delay (byte 0)", ShortName="tempAndVoltageMeasurementInterval", Addr="822",
                      Mode="no", Value="16", Size="2", internalOnly=True,
                      Description="Delay in seconds between different calls to battery voltage measurement task.")
    gen_xml.add_entry(PropName="Battery voltage measurement task delay (byte 1)", ShortName="tempAndVoltageMeasurementInterval", Addr="823",
                      Mode="no", Value="14", Size="2", internalOnly=True,
                      Description="Delay in seconds between different calls to battery voltage measurement task.")

    gen_xml.add_entry(PropName="raw Vproc low voltage threshold (byte 0)", ShortName="vprocLowVoltageThreshold", Addr="824",
                      Mode="no", Value="164", Size="2", internalOnly=True,
                      Description="raw value for ADC window comparator low threshold used for Vproc monitoring during a meter read.")
    gen_xml.add_entry(PropName="raw Vproc low voltage threshold (byte 1)", ShortName="vprocLowVoltageThreshold", Addr="825",
                      Mode="no", Value="8", Size="2", internalOnly=True,
                      Description="raw value for ADC window comparator low threshold used for Vproc monitoring during a meter read.")

    gen_xml.add_entry(PropName="Test task flags (byte 0)", ShortName="test_task_flags", Addr="826",
                      Mode="no", Value="0", Size="2", internalOnly=True,
                      Description="test task flag register (each flag bit is used for a different task)")
    gen_xml.add_entry(PropName="Test task flags (byte 1)", ShortName="test_task_flags", Addr="827",
                      Mode="no", Value="0", Size="2", internalOnly=True,
                      Description="test task flag register (each flag bit is used for a different task)")

    gen_xml.add_entry(PropName="RTC Calibration Interval LSB",            ShortName="RtcCalibrationInterval",         Addr="828", Mode="no", Value="44",  Size="2", internalOnly=True, Description="Time (in seconds) between RTC calibration intervals")
    gen_xml.add_entry(PropName="RTC Calibration Interval MSB",            ShortName="RtcCalibrationInterval",         Addr="829", Mode="no", Value="1",   Size="2", internalOnly=True, Description="Time (in seconds) between RTC calibration intervals")

    gen_xml.add_entry(PropName="RTC Calibration Expiration (hours) LSB",  ShortName="RtcCalibrationExpirationHours",  Addr="830", Mode="no", Value="112", Size="2", internalOnly=True, Description="how long a saved RTC calibration can be used before it expires (units of hours)")
    gen_xml.add_entry(PropName="RTC Calibration Expiration (hours) MSB",  ShortName="RtcCalibrationExpirationHours",  Addr="831", Mode="no", Value="8",   Size="2", internalOnly=True, Description="how long a saved RTC calibration can be used before it expires (units of hours)")

    gen_xml.add_entry(PropName="RTC Calibration Time",                    ShortName="RtcCalibrationMs",               Addr="832", Mode="no", Value="200", Size="1", internalOnly=True, Description="Time (in ms) of an RTC calibration.")

    gen_xml.add_entry(PropName="Heartbeat Mode",                          ShortName="HeartbeatMode",                  Addr="833", Mode="no", Value="0",   Size="1", internalOnly=True, Description="Specifies how the HW heartbeat line functions (0 - toggles every 1 second, 1 - pulses every minute, 2 - high when main loop is awake)")

    gen_xml.add_entry(PropName="Fine Clock Adjust LSB",                   ShortName="FineClockAdjust",                Addr="834", Mode="no", Value="0",   Size="2", internalOnly=True, Description="Adjusts the RTC second counter by the specified number of 32kHz ticks (resets to 0 after adjustment complete)")
    gen_xml.add_entry(PropName="Fine Clock Adjust MSB",                   ShortName="FineClockAdjust",                Addr="835", Mode="no", Value="0",   Size="2", internalOnly=True, Description="Adjusts the RTC second counter by the specified number of 32kHz ticks (resets to 0 after adjustment complete)")

    gen_xml.add_entry(PropName="Voltage Sampling Short Term Avg Count",   ShortName="ShortTermAverageCount",          Addr="836", Mode="no", Value="1",   Size="1", internalOnly=True, Description="How many periodic samples to gather and average for each entry in the short term average voltage storage.")

    gen_xml.add_entry(PropName="Voltage Sampling Medium Term Avg Count",  ShortName="MediumTermAverageCount",         Addr="837", Mode="no", Value="1",   Size="1", internalOnly=True, Description="How many short term average entries to gather and average for each entry in the medium term average voltage storage.")

    gen_xml.add_entry(PropName="Voltage Sampling Long Term Avg Count",    ShortName="LongTermAverageCount",           Addr="838", Mode="no", Value="24",  Size="1", internalOnly=True, Description="How many long term average entries to gather and average for each entry in the long term average voltage storage.")

    gen_xml.add_entry(PropName="Voltage Average Count",                   ShortName="VoltageAvgCount",                Addr="839", Mode="no", Value="24",  Size="1", internalOnly=True, Description="How many medium term average entries to average when calculating tadiranAvg and energizerAvg.")

    gen_xml.add_entry(PropName="Histogram Average Count",                 ShortName="HistogramAvgCount",              Addr="840", Mode="no", Value="24",  Size="1", internalOnly=True, Description="How many short term average entries to gather and average for each entry histogram storage.")

    gen_xml.add_entry(PropName="Temp and Voltage Arrays Initialized",     ShortName="tempVoltageArraysInitialized",   Addr="841", Mode="no", Value="1",   Size="1", internalOnly=True, Description="Set to 0 if the temp/voltage samples/avgs storage needs to be reinitialized")
    
    gen_xml.add_entry(PropName="ADC Temperature Cal Data for 30C LSB",    ShortName="tempAdcCal30C",                  Addr="892", Mode="no", Value="0",   Size="2", internalOnly=True, Description="Copy of the MSP430FR5964 internal temperature ADC calibration value for 30C")
    gen_xml.add_entry(PropName="ADC Temperature Cal Data for 30C MSB",    ShortName="tempAdcCal30C",                  Addr="893", Mode="no", Value="0",   Size="2", internalOnly=True, Description="Copy of the MSP430FR5964 internal temperature ADC calibration value for 30C")

    gen_xml.add_entry(PropName="ADC Temperature Cal Data for 85C LSB",    ShortName="tempAdcCal85C",                  Addr="894", Mode="no", Value="0",   Size="2", internalOnly=True, Description="Copy of the MSP430FR5964 internal temperature ADC calibration value for 85C")
    gen_xml.add_entry(PropName="ADC Temperature Cal Data for 85C MSB",    ShortName="tempAdcCal85C",                  Addr="895", Mode="no", Value="0",   Size="2", internalOnly=True, Description="Copy of the MSP430FR5964 internal temperature ADC calibration value for 85C")


    if output_dir is not None:
        filename = os.path.join(output_dir, xmlFilename+".xml")
        gen_xml.output_to_file(filename)

        checkResults = check_xml_for_errors(filename)
        if checkResults is not None:
            raise RuntimeError("XML Generation Rule Violation when generating {}: {}".format(filename, checkResults))

    return gen_xml


def build_xml_files(fw_family, version_major, version_minor, version_build, appCrc, xml_gen_dir):
    """
    :param conf:
    :param appCrc:
    :param xml_gen_dir:
    :return:
    """

    appCrc = "%04x" % appCrc

    verMajor = "%02d" % version_major
    verMinor = "%02d" % version_minor
    verBuild = "%04d" % version_build

    # versionString = verMajor + "." + verMinor
    # print("- Generating XML Files. Version={}, CRC={}, ProductType={}".format(versionString, appCrc, conf.BuildName))

    if fw_family == 2:
        # build encoder xml files
        encoderSKUs = get_SKUs_of_family(2)

        for encoderSKU in encoderSKUs:
            HW_VERS = get_HW_VERS_of_SKU(encoderSKU)
            for HW_VER in HW_VERS:
                gen_xml_file(sku=encoderSKU, major=verMajor, minor=verMinor, build=verBuild, crc=appCrc,
                             desired_doc_control_rev=_DocCtrlRev, HW_VER=HW_VER, output_dir=xml_gen_dir)

    elif fw_family == 3:
        # build EXT_ALARM xml files
        ExtAlarmSKUs = get_SKUs_of_family(3)

        for ExtAlarmSKU in ExtAlarmSKUs:
            HW_VERS = get_HW_VERS_of_SKU(ExtAlarmSKU)
            for HW_VER in HW_VERS:
                gen_xml_file(sku=ExtAlarmSKU, major=verMajor, minor=verMinor, build=verBuild, crc=appCrc,
                             desired_doc_control_rev=_DocCtrlRev, HW_VER=HW_VER, output_dir=xml_gen_dir)

    elif fw_family == 0:
        # build gas xml files
        gasSKUs = get_SKUs_of_family(0)

        for gasSKU in gasSKUs:
            HW_VERS = get_HW_VERS_of_SKU(gasSKU)
            for HW_VER in HW_VERS:
                gen_xml_file(sku=gasSKU, major=verMajor, minor=verMinor, build=verBuild, crc=appCrc,
                             desired_doc_control_rev=_DocCtrlRev, HW_VER=HW_VER, output_dir=xml_gen_dir)

    elif fw_family == 1:
        # build gas pulse evc xml files
        gasPulseSKUs = get_SKUs_of_family(1)

        for gasPulseSKU in gasPulseSKUs:
            HW_VERS = get_HW_VERS_of_SKU(gasPulseSKU)
            for HW_VER in HW_VERS:
                gen_xml_file(sku=gasPulseSKU, major=verMajor, minor=verMinor, build=verBuild, crc=appCrc,
                             desired_doc_control_rev=_DocCtrlRev, HW_VER=HW_VER, output_dir=xml_gen_dir)

    elif fw_family ==99:
        gasPulseSKUs = get_SKUs_of_family(1)

        for gasPulseSKU in gasPulseSKUs:
            HW_VERS = get_HW_VERS_of_SKU(gasPulseSKU)
            for HW_VER in HW_VERS:
                gen_xml_file(sku=gasPulseSKU, major=verMajor, minor=verMinor, build=verBuild, crc=appCrc,
                             desired_doc_control_rev=_DocCtrlRev, HW_VER=HW_VER, output_dir=xml_gen_dir)



if __name__ == "__main__":
    build_xml_files(fw_family=2, version_major=1, version_minor=6, version_build=49, appCrc=0, xml_gen_dir="C:\\temp\\xmls")
