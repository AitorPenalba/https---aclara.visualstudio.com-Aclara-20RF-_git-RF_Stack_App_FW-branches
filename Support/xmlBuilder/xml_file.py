# -*- coding: cp1252 -*-
from lxml import etree


_includeFirmwareTestSuiteEntries = True
_verifiedByValue = None
_includeInternalCommands = True


class XMLFile(object):
    def __init__(self, DocControlRev, DocControlID, PartNumber, FirmwareVersion, CatalogNumber):
        self.root = etree.Element("ConfigProfileEntries", DocControlRev=DocControlRev, DocControlID=DocControlID,
                                   PartNumber=PartNumber, FirmwareVersion=FirmwareVersion, CatalogNumber=CatalogNumber)

    def add_entry(self, PropName, ShortName, Addr, Mode, Value, Size, internalOnly, Description):
        IsBitField = "False"
        BitPosition = "0"
        BitLength = "8"

        if Mode == "c":
            IsConfigurable = "True"
            IsAuditible = "False"
            IsValidated = "False"
            ConfigurationValue = Value
            ValidationValue = None
        elif Mode == "ca":
            IsConfigurable = "True"
            IsAuditible = "True"
            IsValidated = "False"
            ConfigurationValue = Value
            ValidationValue = None
        elif Mode == "va":
            IsConfigurable = "False"
            IsAuditible = "True"
            IsValidated = "True"
            ConfigurationValue = None
            ValidationValue = Value
        elif Mode == "v":
            IsConfigurable = "False"
            IsAuditible = "False"
            IsValidated = "True"
            ConfigurationValue = None
            ValidationValue = Value
        elif Mode == "no":
            IsConfigurable = "False"
            IsAuditible = "False"
            IsValidated = "False"
            ConfigurationValue = Value
            ValidationValue = None
        else:
            raise ValueError("Invalid Mode")

        if (_includeInternalCommands == False) and (internalOnly == True):
            pass
        else:
            if internalOnly and Mode != "no":
                print("Warning!!!! Mode is not 'no' on an internal only command! (" + PropName + ")")

        entry = etree.SubElement(self.root, "Entry")
        propName = etree.SubElement(entry, "PropertyName")
        propName.text = PropName
        if _includeFirmwareTestSuiteEntries:
            shortName = etree.SubElement(entry, "ShortName")
            shortName.text = ShortName
        add = etree.SubElement(entry, "Address")
        add.text = Addr
        isBit = etree.SubElement(entry, "IsBitField")
        isBit.text = IsBitField
        bitPos = etree.SubElement(entry, "BitPosition")
        bitPos.text = BitPosition
        bitLen = etree.SubElement(entry, "BitLength")
        bitLen.text = BitLength
        isConfig = etree.SubElement(entry, "IsConfigurable")
        isConfig.text = IsConfigurable
        configValue = etree.SubElement(entry, "ConfigurationValue")
        configValue.text = ConfigurationValue
        isValid = etree.SubElement(entry, "IsValidated")
        isValid.text = IsValidated
        validVal = etree.SubElement(entry, "ValidationValue")
        validVal.text = ValidationValue
        isAudit = etree.SubElement(entry, "IsAuditible")
        isAudit.text = IsAuditible
        if _includeFirmwareTestSuiteEntries:
            byteLen = etree.SubElement(entry, "ByteLength")
            byteLen.text = Size
        verify = etree.SubElement(entry, "VerifiedBy")
        if _verifiedByValue is not None:
            verify.text = _verifiedByValue

        if _includeFirmwareTestSuiteEntries:
            description = etree.SubElement(entry, "Description")
            description.text = Description

    def output_to_file(self, filename):
        xmlString = etree.tostring(self.root)
        xmlString = self._get_pretty_printed_xml_string(xmlString.decode())

        prettyPrinted = self._get_pretty_printed_xml_string(etree.tostring(self.root).decode())
        prettyPrinted = '<?xml version="1.0" encoding="UTF-8"?>\n' + prettyPrinted
        f = open(filename, 'w')
        f.write(prettyPrinted)
        f.close()

    @staticmethod
    def _get_pretty_printed_xml_string(xmlString):
        xmlString = xmlString.replace("><", ">\n<")
        xmlString = xmlString.replace("<Entry>", "\r<Entry>")
        return xmlString


if __name__ == "__main__":
    pass
