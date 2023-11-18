from eth_utils import keccak
from rlp import encode
import rlp
from pprint import pprint

def parse_node(node):
    if len(node) == 17:
        # Branch node
        return {i: node[i] for i in range(16)}, node[16]
    elif len(node) == 2:
        # Extension or leaf node
        key_segment = node[0]
        if len(key_segment) % 2 == 1:
            # Odd length, drop the first byte
            key_segment = key_segment[1:]
        else:
            # Even length, drop the first two bytes
            key_segment = key_segment[2:]
        return key_segment, node[1]
    raise Exception("Invalid node format")


def bytes_to_nibbles(byte_array):
    nibbles = []
    for byte in byte_array:
        nibbles.append(byte >> 4)  # High nibble
        nibbles.append(byte & 0x0F)  # Low nibble
    return nibbles


def decode_to_node(encoded_node):
    if isinstance(encoded_node, str):
        encoded_node = bytes.fromhex(encoded_node[2:])
    return rlp.decode(encoded_node)


def parse_proof(proof_nodes):
    parsed_proof = []
    for node in proof_nodes:
        if isinstance(node, list):
            parsed_proof.append(node)
        else:
            parsed_proof.append(decode_to_node(node))
    return parsed_proof


def verify_proof(expected_root_hash, key, proof_nodes):
    key = keccak(key)
    key_nibbles = bytes_to_nibbles(key)
    parsed_proof = parse_proof(proof_nodes)
    node_hash = expected_root_hash

    for node in parsed_proof:
        if keccak(encode(node)) != node_hash:
            raise Exception("Invalid proof node hash")

        if len(node) == 17:
            index = key_nibbles.pop(0)
            node_hash = node[index]
        else:
            _, value = parse_node(node)
            return value

    raise Exception("Leaf not found")


storage_hash = "0x7b2170eb1342082d7752a9670edb1c3f18bf23b8af1d23136a4627fea340cf0f"

key = "0x"+"00"*32
proof = [
    "0xf90211a0f4c5c0c86aed65a5eb361f687474b7134df341bd5535da265127f94152b9c0f9a0b15d7dd0b85c227c0e2a16576971e2bf9150cd305e44d7add7ac52c6f1505305a0ccb5c6e6c6be7542cb5768ed513ee29688e6448ab0471f7beec0e45b15635083a02d756325779ce715d5cfcacfd9edb96d5dc0036b9db03dd57d4e23845ae071f5a07cfd68c326cbb2d9ccf3ac0726720c2c6e065540f585f2d47c7d1bfe2f29ecd5a0676c8d449bbf42c928feeaefd68df2bc5109401a12b8a3146eb37628c7131b7aa06212a604ab8080ff633524c47ca13bb82d5238258115f6929474b9cd72405f08a0bf7b2b798254ef5ee1178acd28b05023f77bedd979a0773e312546048e775dfba06e2a644cc77bd546cd878d20f6528933e8d9c6edf9944d296d181e56c0013285a0dc31d89eb40c004b2a1fd0704699db24f0860c2955b866bb549e6c20dd23b503a039eefb678fb3b701b36e1164c8a41d2ce2b255e3ca72fa61f8a333faaf6ecb79a04690272414baf906827c2776b310317791c7ad60d85edac82e35c0fe7573ae1ea0edef95067258d17064a9a0daccc21e17fa619cbb9d7b3ba6b7c09558fd623535a06bf61870e5080bc2a2763c3934119b39c58b5dcb532522326c4cfe9b34019656a0a6fb36e4ec5756150c41a3bf6955003aa4db46d97338950847542992141c78cea0f15713c1ac1d533f188ee9f5c27f489e16767e4705942fd0e8c0ba6782b0f28b80",
    "0xf90211a07ff1ad5d6a43bd414fbd35dc030b19739fc7ce61f7060e61d9d25461e3cad131a09fe96eeaf16bf4c82e46c85d9192a7d3d9b01bf8d4d0b4efedf14dc1842926dda0e486bfe52e93de018005124731ac8c58e5b535d49078fac5b7158d62b8049543a0b961b2a7a3d143dd4de2baceaf3847c08c0d4da5c0968ce9cc3f95b5a67b0aeba0a981d536420b09ca2bd13ddaba198f7cf96d29fffa448e788fdf05ec81439dd6a08787430e97b56bfb38286ffec95f98c26f010a82ceeb35436ae3802cf9e6e4f6a05a01872d0f51417ad6395dccadcd1380b4fcd92374fef3174d061de4f098c738a08bb5039f2ec6dd49cce2601d357b9e44974b9fcfadeaf5eeb30f4d2c164df5a7a0521969c1a14f574bb2d24fe03bb693d04fa2e031b795b1e17bc8fde51a29b7c7a05fcfe389598c704d9a79d7e2747e80a536d4ac6b9428b842d62f011704e5d5b3a04dbe64d816606184f3cd89af4b1c513b38957463519a42975b4732530f99b6f9a011dd5277ca38900f9155c55bda46572b93aefd2cbd282a6de8d796786cf71b0da03f8d427ef8d722be5cc7f73c1e1ed6ceb37e131b0a4d610c2f370634eda820faa06da1a8b440f2a02e82353a83edf1f2b7b8fced703fde4421d8203a3dedc63bd7a046f0f0cc10317e9912415f3f6332ebca496a0dcf57be0b38b3a466834550c6d1a09bcb37de0eb09160a1052ec8c28de51f6959ec78baf325d19843941fb82f0d9680",
    "0xf90211a06bc87a42d30e98f0f4984f9ecc9aa8322b999f4e50901445fe9195050e7e1e17a0d30567927d08ea129eb588e38a58864c75b131f7293e89d7fa9ec239bcc050a4a03286e4979fc4f2a0377e3bb4d7a15474069a1345601aa1dfa6903a71d5e3c7f4a0b94d1a0b44fede2c48f8752c3b5d7810a066dd2d0e96f3ac9dfe1d5bf4265d00a05ea7794b5c641424189d0fcab8979221c2f553af569cdd02c766ef112d718781a0370550869de2818bf19d20cefb74df7d0e88164b032d57cec782daf4bea33058a03d0071e8b6533945c941cfd59556f189521102fd708870ee35c6be5a6faa8c8aa04ef305c8f3370f24aa34c06c25b660146946c3d1d238951a53d7345472a3afc0a0669c576f48b2a58150ef0e78f915703b29651143f673ab63c05ddba8c89a6269a00bb3c7e81ac31293bf191ccf3e57d2024fb59dd49a6d990d726a142c423d6590a0270660955e6ef67301960800da56908123321e6a91f1eaf14f103d6a9966642ea05f512b96e298b7496ab3cbd94a2e6c89b395cb48197ab3c178e259be82a1e8d6a029f5199e170b94fec77a12a3fbcfb149982175b3c96f06c3a976af30e6bf2fa0a0d15396091778ff2235bb615f69c6613408f6669040ecbebc953433eae2ea351ba09571bbbb82550ffdd4d9508aa7b023bbfd452fb0173d63fe89d102703ad7db5ba0cbe3cab85e1208b5ff3decf2893dbf6c73f7704275707acee728b77581f4df4880",
    "0xf90211a0890a0a28a66f93fb32d6b0309a8afa42a1191030df212303d795c236d6b6f43ba00de1c486b9e404934cc6c8fa4c85ec6a7c2ff0d8ab2c8a0e5aa70e9d0507e4c7a014692a00520aa8f390f541ad8bea6e2686db827b075bc238da73290d9b04183ea01b37c867014890000fdb76591c89e21aa051f9d5b503682372793015debca95aa0cb87cf3cfd57bf94e91c1446fd9503c1e45ee76d8ba3d70bfd6663809b827e40a05e893da90fe58ddfe21bace6560aefae0862079a83ac813f1f61167a31d12164a0a85e413799af74ee3b68c4e6f1e8eac6403a26341b7de20ccf3ef5fd1700f09ea0a13847750e17aa186e7dbf317260a56c07ee39af032556426e6b697d0918ae49a03ac4997ca1ebbbcac84e9a6b34dccf8f0c79f1e89b109459a4e52e555c83d2d6a0508a64c5159faeebf7a00c69d3e2e80fce8b7626dd35fffb6e6216658b283cd5a08dd78fbe96aeff73808c637d2bf46edb81ebb3d3860069e86a08f66ba46c19aea03920bd60ef1a4a7eb92df45f015c427a513c4f4a32b8f1a27e8c849bdc03a339a0bb107409cf01bf7096f627e20725fc5b26a6ae1f674a2f6bb965eef32296b933a0f1839c6cac38ba3646d58390455ac80efa91b3b8e13b6b7dac837761eee5f4a9a0217bafc0c218a96cf1ab757e9a5bae200f1634075de64536661dc85c22c02e3ca0e19baca6a00e41cba4e470336c48f1ff11c633cc888401f84ae2a6e2d63ad1a580",
    "0xf871a033df9afcd45aefdb9009dc7be4b83a680a7a454a40aceff0a8d909117bddeec7808080808080a09442848737ada0c1ab83b3349390511f8eb33ae7280589d86fb6114f232859de808080808080a0954e8545cb42ac37f2ef0f79f745a03c660dd143a7b621132385060a8345e8218080",
    "0xe09e3cd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e56301"
]

storage_hash_bytes = bytes.fromhex(storage_hash[2:])
key_bytes = bytes.fromhex(key[2:])

value = verify_proof(storage_hash_bytes, key_bytes, proof)
print(value)
