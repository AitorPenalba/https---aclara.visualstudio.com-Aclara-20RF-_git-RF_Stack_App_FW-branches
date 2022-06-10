import xml.etree.ElementTree as ET


def check_xml_for_errors(xmlfile):
    """
    This script checks the manufacturing XML files for errors that will break Harvey's importer. The following rules are checked:

    1) Only tags of type <Entry> should exist.
    2) An <Entry> tag should not have any attributes (<Entry myAttribute=5> would be invalid)
    3) Only the following sublines should exist: PropertyName, ShortName, Address, IsBitField, BitPosition, BitLength, IsConfigurable, ConfigurationValue, IsValidated, ValidationValue, IsAuditible, ByteLength, VerifiedBy, Description
    4) One and only one of the sublines in rule 2 should exist.
    5) The PropertyName field should be unique - even for multibyte variables
    6) Only addresses between 0 and 1023 (inclusive) are allowed.
    7) Entries should be listes so that the address fields are in ascending order.
    8) There should be no duplicate addresses.
    9) The <IsBitField> tag must be False (we don't use it)
    10) The <BitPosition> tag must be 0 (we don't use it)
    11) The <BitLength> tag must be 8 (we don't use it)
    12) The <IsConfigurable> tag must be True or False
    13) The <IsValidated> tag must be True or False
    14) The <IsAuditible> tag must be True or False
    15) Configuration value must be present when IsConfigurable is True, and must be an 8-bit unsigned integer.
    16) Validation value must be present when IsValidated is True, and must be an 8-bit unsigned integer.
    17) Validation value should not be present when IsValidated is False
    18) ByteLength must be a positive integer
    19) All ByteLength entries for the same shortName must be the same.
    20) All Description entries for the same shortName must be the same.
    21) The number of entries with the same short name should match the ByteLength for those entries
    
    
    """
    
    propertyNames = set()
    shortNameInfo = {}

    lastAddress = -1
    
    tree = ET.parse(xmlfile)

    root = tree.getroot()

    for child in root:
        if child.tag != "Entry":
            # rule #1
            return("Invalid child tag! Only 'Entry' should exist!")

        if len(child.attrib) != 0:
            # rule #2
            return("'Entry' tags should not have any attributes...they should just be <Entry>")

        if len(child) != 14:
            # rule #4
            return("There are the wrong number of properties within an <Entry> tag")

        for lineItem in child:
            if lineItem.tag == "PropertyName":
                propertyName = lineItem.text
                # PropertyNames should be unique
                if propertyName not in propertyNames:
                    propertyNames.add(propertyName)
                else:
                    # rule #5
                    return("Duplicate Property Name!!!!  '{}'".format(lineItem.text))

            elif lineItem.tag == "ShortName":
                shortName = lineItem.text
                if shortName not in shortNameInfo:
                    shortNameInfo[shortName] = [1, None, None]  # num entries, ByteLength, Description
                else:
                    shortNameInfo[shortName][0] += 1
            
            elif lineItem.tag == "Address":
                addr = int(lineItem.text)

                if (addr < 0) or (addr > 1023):
                    # rule #6
                    return("Out of bound address!!!! ({})".format(addr))
                elif (addr <= lastAddress):
                    # rules #7 and #8
                    return("Out of order or duplicated addresses!!!! ({})".format(addr))
                else:
                    lastAddress = addr

            elif lineItem.tag == "IsBitField":
                if lineItem.text != "False":
                    # rule #9
                    return("Invalid IsBitField entry! ({})".format(propertyName))

            elif lineItem.tag == "BitPosition":
                if lineItem.text != "0":
                    # rule #10
                    return("Invalid BitPosition entry! ({})".format(propertyName))

            elif lineItem.tag == "BitLength":
                if lineItem.text != "8":
                    # rule #11
                    return("Invalid BitLength entry! ({})".format(propertyName))

            elif lineItem.tag == "IsConfigurable":
                if lineItem.text == "True":
                    isConfigurable = True
                elif lineItem.text == "False":
                    isConfigurable = False
                else:
                    # rule #12
                    return("Invalid IsConfigurable entry! ({})".format(propertyName))
                
            elif lineItem.tag == "ConfigurationValue":
                if isConfigurable:
                    configurationValue = int(lineItem.text)

                    if (configurationValue < 0) or (configurationValue > 255):
                        # rule #15
                        return("Out of bounds configuration value! ({})".format(propertyName))

            elif lineItem.tag == "IsValidated":
                if lineItem.text == "True":
                    isValidated = True
                elif lineItem.text == "False":
                    isValidated = False
                else:
                    # rule #13
                    return("Invalid IsValidated entry! ({})".format(propertyName))
                
            elif lineItem.tag == "ValidationValue":
                if isValidated:
                    validationValue = int(lineItem.text)

                    if (validationValue < 0) or (validationValue > 255):
                        # rule #16
                        return("Out of bounds validation value! ({})".format(propertyName))
                    
                elif lineItem.text is not None:
                    # rule #17
                    return("Validation Value is present when IsValidated is False ({})".format(propertyName))
            
            elif lineItem.tag == "IsAuditible":
                if lineItem.text == "True":
                    isAuditible = True
                elif lineItem.text == "False":
                    isAuditible = False
                else:
                    # rule #14
                    return("Invalid IsAuditible entry! ({})".format(propertyName))

            elif lineItem.tag == "ByteLength":
                byteLength = int(lineItem.text)

                if byteLength < 0:
                    # rule #18
                    return("ByteLength must be a positive integer! ({})".format(propertyName))
                
                if shortNameInfo[shortName][1] is None:
                    shortNameInfo[shortName][1] = byteLength
                elif byteLength != shortNameInfo[shortName][1]:
                    # rule #19
                    return("All multibyte byte length entries must be the same value ({})".format(propertyName))

            elif lineItem.tag == "VerifiedBy":
                pass
            
            elif lineItem.tag == "Description":
                description = lineItem.text

                if shortNameInfo[shortName][2] is None:
                    shortNameInfo[shortName][2] = description
                elif description != shortNameInfo[shortName][2]:
                    # rule #20
                    return("All multibyte description entries must be the same value ({})".format(propertyName))

            else:
                # rule #3
                return("Unknown line item!!!")

    for key in shortNameInfo:
        if shortNameInfo[key][0] != shortNameInfo[key][1]:
            # rule #21
            return("Number of byte length entries doesn't match byteLength value")

    



if __name__ == "__main__":
    returnVal = check_xml_for_errors(xmlfile="3451-XXX-XB__17-013E__V01.02__182__1PORT__EXT_ALARMS__STD_RANGE.xml")

    if returnVal is not None:
        raise RuntimeError(returnVal)
    else:
        print("No errors found!")


    
