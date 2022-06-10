
class Structure(object):
    _fields = []

    def __init__(self, *args):
        if len(args) != len(self._fields):
            raise TypeError('Expected {} arguments'.format(len(self._fields)))
        for name, value in zip(self._fields, args):
            setattr(self, name, value)


class MtuConfDetails(Structure):
    _fields = ['partNumber', 'portType', 'meterType', 'mtuRange', 'hasRDD', 'antennaType', 'isRemoteGas', 'formCReed', 'flowIsCW', 'FW_FAMILY', 'HW_IDs', 'description']


fw_family_to_mtuTypes = {
    0: set([0]+[n for n in range(196, 214)]+[215]+[218, 219]+[221]),
    1: set([1]+[216, 217]),
    2: set([2]+[180, 181]+[183]+[185, 187, 188, 189]+[191]+[193, 195]+[224]),
    3: set([3] + [182, 184, 186, 190, 192, 194]),
}


SKUs_to_MTUTypes = {
    '3451-XXX-RB': 180,
    '3451-XXX-DB': 181,
    '3451-XXX-XB': 182,
    '3450-XXX-ZCV': 183,
    '3451-XXX-XCV': 184,
    '3452-XXX-DB': 185,
    '3452-XXX-XB': 186,
    '3451-XXX-DCV': 187,
    '3451-XXX-RBW': 188,
    '3451-XXX-RBW-A': 188,
    '3451-XXX-DBW': 189,
    '3451-XXX-DBW-A': 189,
    '3451-XXX-XBW': 190,
    '3451-XXX-XBW-A': 190,
    '3450-XXX-ZCVW': 191,
    '3450-XXX-ZCVW-A': 191,
    '3451-XXX-XCVW': 192,
    '3451-XXX-XCVW-A': 192,
    '3452-XXX-DBW': 193,
    '3452-XXX-DBW-A': 193,
    '3452-XXX-XBW': 194,
    '3452-XXX-XBW-A': 194,
    '3451-XXX-DCVW': 195,
    '3451-XXX-DCVW-A': 195,
    '3531-010-RBS2': 196,
    '3551-031-RBS2': 197,
    '3541-021-RBS2': 198,
    '3541-022-RBS2': 199,
    '3571-015-RBS2': 200,
    '3581-055-RBS2W-K': 201,
    # '3561-051-RBS2': 202,   # gas SKU which we don't currently support
    '3531-010-RBS2W': 203,
    '3551-031-RBS2W': 204,
    '3541-021-RBS2W': 205,
    '3541-022-RBS2W': 206,
    '3571-015-RBS2W': 207,
    # '3561-051-RBS2W': 208,  # gas SKU which we don't currently support
    '3621-010-RBS2W': 209,
    '3621-010-RBS2W-A': 209,
    '3621-021-RBS2W': 210,
    '3621-021-RBS2W-A': 210,
    '3621-022-RBS2W': 211,
    '3621-022-RBS2W-A': 211,
    '3621-031-RBS2W': 212,
    '3621-031-RBS2W-A': 212,
    '3621-019-RBS2W': 213,
    '3621-019-RBS2W-A': 213,
    # '3621-051-RBS2W': 214,    # gas SKU which we don't currently support
    '3621-051-RBS2W-A': 214,
    '3621-039-RBS2W': 215,
    '3621-039-RBS2W-A': 215,
    '3621-000-RBS2W': 216,
    # '3621-000-RBS2W-A': 216,  # external antenna variant, which we don't currently sell
    '3622-000-RBS2W': 217,
    # '3622-000-RBS2W-A': 217,  # external antenna variant, which we don't currently sell
    # '3531-010-RRS2': 218,     # gas RDD unit which we don't currently support
    # '3531-010-RRS2W': 219,    # gas RDD unit which we don't currently support
    # '3621-010-RRS2W': 220,    # gas RDD unit which we don't currently support
    # '3621-010-RRS2W-A': 220,  # gas RDD unit which we don't currently support
    '3621-029-RBS2W': 221,
    '3621-029-RBS2W-A': 221,
    '3452-XXX-RBW': 224,
    '3452-XXX-RBW-A': 224
}


MTUTypes_to_Confs = {
    # ----------------- |  partNumber                    |  portType  | meterType         | mtuRange | hasRDD  | antennaType | isRemoteGas | formCReed | flowIsCW | FW_FAMILY | HW IDs | Description
    180: MtuConfDetails({0: "17-013", 1: "19-006S"},      "1PORT",     "PULSE",            "STD",      False,    "1",         None,         None,       None,      "2",        [0, 1],  'On-Demand Water - Standard Range Single Port Pulse'),
    181: MtuConfDetails({0: "17-013", 1: "19-006S"},      "1PORT",     "ENCODER",          "STD",      False,    "1",         None,         None,       None,      "2",        [0, 1],  'On-Demand Water - Standard Range Single Port Encoder'),
    182: MtuConfDetails({0: "17-013E", 1: "19-006ES"},    "1PORT",     "EXT_ALARMS",       "STD",      False,    "1",         None,         None,       None,      "3",        [0, 1],  'On-Demand Water - Standard Range Single Port Extended Alarms'),
    183: MtuConfDetails({0: "17-013", 1: "19-006"},       "0PORT",     "NO_METER",         "STD",      True,     "1",         None,         None,       None,      "2",        [0, 1],  'On-Demand Water - Standard Range Standalone RDD'),
    184: MtuConfDetails({0: "17-013E", 1: "19-006ES"},    "1PORT",     "EXT_ALARMS",       "STD",      True,     "1",         None,         None,       None,      "3",        [0, 1],  'On-Demand Water - Standard Range Single Port Extended Alarms with RDD'),
    185: MtuConfDetails({0: "17-013", 1: "19-006D"},      "2PORT",     "ENCODER",          "STD",      False,    "1",         None,         None,       None,      "2",        [0, 1],  'On-Demand Water - Standard Range Dual Port Encoder'),
    186: MtuConfDetails({0: "17-013E",  1: "19-006ED"},   "2PORT",     "EXT_ALARMS",       "STD",      False,    "1",         None,         None,       None,      "3",        [0, 1],  'On-Demand Water - Standard Range Dual Port Extended Alarms'),
    187: MtuConfDetails({0: "17-013", 1: "19-006S"},      "1PORT",     "ENCODER",          "STD",      True,     "1",         None,         None,       None,      "2",        [0, 1],  'On-Demand Water - Standard Range Dual Port Extended Alarms'),
    188: MtuConfDetails({0: "17-013", 1: "19-006S"},      "1PORT",     "PULSE",            "EXT",      False,    "1",         None,         None,       None,      "2",        [0, 1],  'On-Demand Water - Extended Range Single Port Pulse'),
    189: MtuConfDetails({0: "17-013", 1: "19-006S"},      "1PORT",     "ENCODER",          "EXT",      False,    "1",         None,         None,       None,      "2",        [0, 1],  'On-Demand Water - Extended Range Single Port Encoder'),
    190: MtuConfDetails({0: "17-013E", 1: "19-006ES"},    "1PORT",     "EXT_ALARMS",       "EXT",      False,    "1",         None,         None,       None,      "3",        [0, 1],  'On-Demand Water - Extended Range Single Port Extended Alarms'),
    191: MtuConfDetails({0: "17-013", 1: "19-006"},       "0PORT",     "NO_METER",         "EXT",      True,     "1",         None,         None,       None,      "2",        [0, 1],  'On-Demand Water - Extended Range Standalone RDD'),
    192: MtuConfDetails({0: "17-013E", 1: "19-006ES"},    "1PORT",     "EXT_ALARMS",       "EXT",      True,     "1",         None,         None,       None,      "3",        [0, 1],  'On-Demand Water - Extended Range Single Port Extended Alarms with RDD'),
    193: MtuConfDetails({0: "17-013", 1: "19-006D"},      "2PORT",     "ENCODER",          "EXT",      False,    "1",         None,         None,       None,      "2",        [0, 1],  'On-Demand Water - Extended Range Dual Port Encoder'),
    194: MtuConfDetails({0: "17-013E", 1: "19-006ED"},    "2PORT",     "EXT_ALARMS",       "EXT",      False,    "1",         None,         None,       None,      "3",        [0, 1],  'On-Demand Water - Extended Range Dual Port Extended Alarms'),
    195: MtuConfDetails({0: "17-013", 1: "19-006S"},      "1PORT",     "ENCODER",          "EXT",      True,     "1",         None,         None,       None,      "2",        [0, 1],  'On-Demand Water - Extended Range Single Port Encoder with RDD'),
    196: MtuConfDetails({0: "17-020-AM", },               "1PORT",     "GAS_DIRECT",       "STD",      False,    "1",         False,        False,      False,     "0",        [0, ],   'On-Demand Gas - Direct Mount,  Standard Range  (American Residential)'),
    197: MtuConfDetails({0: "17-021", },                  "1PORT",     "GAS_DIRECT",       "STD",      False,    "1",         False,        False,      False,     "0",        [0, ],   'On-Demand Gas - Direct Mount,  Standard Range  (Sprague Slant Face Residential)'),
    198: MtuConfDetails({0: "17-020-R11", },              "1PORT",     "GAS_DIRECT",       "STD",      False,    "1",         False,        False,      True,      "0",        [0, ],   'On-Demand Gas - Direct Mount,  Standard Range  (Rockwell R275)'),
    199: MtuConfDetails({0: "17-020-R18", },              "1PORT",     "GAS_DIRECT",       "STD",      False,    "1",         False,        False,      True,      "0",        [0, ],   'On-Demand Gas - Direct Mount,  Standard Range  (Rockwell R415)'),
    200: MtuConfDetails({0: "17-023", },                  "1PORT",     "GAS_DIRECT",       "STD",      False,    "2",         False,        False,      False,     "0",        [0, ],   'On-Demand Gas - Direct Mount,  Standard Range  (American Commercial)'),
    201: MtuConfDetails({0: "17-022B", },                 "1PORT",     "GAS_DIRECT",       "EXT",      False,    "3",         False,        False,      False,     "0",        [0, ],   'On-Demand Gas - Direct Mount,  Extended Range  (Dresser B3 Under Glass)'),
    202: MtuConfDetails({0: "17-024", },                  "1PORT",     "GAS_DIRECT",       "STD",      False,    "1",         False,        False,      True,      "0",        [0, ],   'On-Demand Gas - Direct Mount,  Standard Range  (Dresser Z)'),
    203: MtuConfDetails({0: "17-020-AM", },               "1PORT",     "GAS_DIRECT",       "EXT",      False,    "1",         False,        False,      False,     "0",        [0, ],   'On-Demand Gas - Direct Mount,  Extended Range  (American Residential)'),
    204: MtuConfDetails({0: "17-021", },                  "1PORT",     "GAS_DIRECT",       "EXT",      False,    "1",         False,        False,      False,     "0",        [0, ],   'On-Demand Gas - Direct Mount,  Extended Range  (Sprague Slant Face Residential)'),
    205: MtuConfDetails({0: "17-020-R11", },              "1PORT",     "GAS_DIRECT",       "EXT",      False,    "1",         False,        False,      True,      "0",        [0, ],   'On-Demand Gas - Direct Mount,  Extended Range  (Rockwell R275)'),
    206: MtuConfDetails({0: "17-020-R18", },              "1PORT",     "GAS_DIRECT",       "EXT",      False,    "1",         False,        False,      True,      "0",        [0, ],   'On-Demand Gas - Direct Mount,  Extended Range  (Rockwell R415)'),
    207: MtuConfDetails({0: "17-023", },                  "1PORT",     "GAS_DIRECT",       "EXT",      False,    "2",         False,        False,      False,     "0",        [0, ],   'On-Demand Gas - Direct Mount,  Extended Range  (American Commercial)'),
    208: MtuConfDetails({0: "17-024", },                  "1PORT",     "GAS_DIRECT",       "EXT",      False,    "1",         False,        False,      True,      "0",        [0, ],   'On-Demand Gas - Direct Mount,  Extended Range  (Dresser Z)'),
    209: MtuConfDetails({0: "17-013G", 1: "19-006G"},     "1PORT",     "GAS_REMOTE",       "EXT",      False,    "1",         True,         True,       False,     "0",        [0, 1],  'On-Demand Gas - Remote,  Extended Range  (American Residential)'),
    210: MtuConfDetails({0: "17-013G", 1: "19-006G"},     "1PORT",     "GAS_REMOTE",       "EXT",      False,    "1",         True,         True,       True,      "0",        [0, 1],  'On-Demand Gas - Remote,  Extended Range  (Rockwell R275 Residential)'),
    211: MtuConfDetails({0: "17-013G", 1: "19-006G"},     "1PORT",     "GAS_REMOTE",       "EXT",      False,    "1",         True,         True,       True,      "0",        [0, 1],  'On-Demand Gas - Remote,  Extended Range  (Rockwell R415 Residential)'),
    212: MtuConfDetails({0: "17-013G", 1: "19-006G"},     "1PORT",     "GAS_REMOTE",       "EXT",      False,    "1",         True,         True,       False,     "0",        [0, 1],  'On-Demand Gas - Remote,  Extended Range  (Sprague)'),
    213: MtuConfDetails({0: "17-013G", 1: "19-006G"},     "1PORT",     "GAS_REMOTE",       "EXT",      False,    "1",         True,         False,      False,     "0",        [0, 1],  'On-Demand Gas - Remote Mount,  Extended Range  (American Curb)'),
    214: MtuConfDetails({0: "17-013G", 1: "19-006G"},     "1PORT",     "GAS_REMOTE",       "EXT",      False,    "1",         True,         False,      False,     "0",        [0, 1],  'On-Demand Gas - Remote,  Extended Range  (Dresser Z Residential)'),
    215: MtuConfDetails({0: "17-013G", 1: "19-006G"},     "1PORT",     "GAS_REMOTE",       "EXT",      False,    "1",         True,         True,       True,      "0",        [0, 1],  'On-Demand Gas - Curb,  Extended Range  (Itron Residential)'),
    216: MtuConfDetails({0: "17-013P", 1: "19-006P"},     "1PORT",     "REMOTE_GAS_PULSE", "EXT",      False,    "1",         True,         False,      False,     "1",        [0, 1],  'On-Demand Gas - Remote Mount ,  Extended Range  (1 Port Pulse Input)'),
    217: MtuConfDetails({0: "17-013P", 1: "19-006P"},     "2PORT",     "REMOTE_GAS_PULSE", "EXT",      False,    "1",         True,         False,      False,     "1",        [0, 1],  'On-Demand Gas - Remote Mount ,  Extended Range  (2 Port Pulse Input)'),
    218: MtuConfDetails({0: "17-020", },                  "1PORT",     "GAS_DIRECT",       "STD",      True,     "1",         False,        False,      False,     "0",        [0, ],   'On-Demand Gas - Direct Mount,  Standard Range  (American Residential with RDD)'),
    219: MtuConfDetails({0: "17-020", },                  "1PORT",     "GAS_DIRECT",       "EXT",      True,     "1",         False,        False,      False,     "0",        [0, ],   'On-Demand Gas - Direct Mount,  Extended Range  (American Residential with RDD)'),
    220: MtuConfDetails({0: "17-013G", 1: "19-006G"},     "1PORT",     "GAS_REMOTE",       "EXT",      True,     "1",         True,         False,      False,     "0",        [0, 1],  'On-Demand Gas - Remote Mount,  Extended Range  (American Residential with RDD)'),
    221: MtuConfDetails({0: "17-013G", 1: "19-006G"},     "1PORT",     "GAS_REMOTE",       "EXT",      False,    "1",         True,         False,      False,     "0",        [0, 1],  'On-Demand Gas - Curb,  Extended Range  (Sensus Residential)'),
    224: MtuConfDetails({0: "17-013", 1: "19-006"},       "2PORT",     "PULSE",            "EXT",      False,    "1",         None,         None,       None,      "2",        [0, 1],  'On-Demand Water - Extended Range Dual Port Pulse')
}


def get_SKUs_of_family(fw_family):
    SKUs = []
    for sku in SKUs_to_MTUTypes:
        if SKUs_to_MTUTypes[sku] in fw_family_to_mtuTypes[fw_family]:
            SKUs.append(sku)
    return SKUs


def get_HW_VERS_of_SKU(sku):
    confDetails = MTUTypes_to_Confs[SKUs_to_MTUTypes[sku]]
    return confDetails.HW_IDs


if __name__ == '__main__':
    conf = MTUTypes_to_Confs[221]
