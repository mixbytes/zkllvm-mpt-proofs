import json
import sys
from web3 import Web3, WebsocketProvider
from web3.middleware import geth_poa_middleware

from eth_utils import (
    keccak,
    to_hex,
    to_bytes,
)
import rlp
from rlp.sedes import (
    Binary,
    big_endian_int,
)
from trie import (
    HexaryTrie,
)
from web3._utils.encoding import (
    pad_bytes,
)
from pprint import pprint


# Ethereum node URL
node_url = "wss://eth-mainnet.g.alchemy.com/v2/p7sadjYi_zcgAhMKGvcS-aaksaWkdzdN"

# Contract address and storage slot
#contract_address = "0xB4e16d0168e52d35CaCD2c6185b44281Ec28C9Dc"
contract_address = "0xd19d4B5d358258f05D7B411E21A1460D11B0876F"
storage_slot = 0  # Replace with your storage slot number

# Block number (optional, set to 'latest' by default)
block_number = 17973376

# Connect to Ethereum node
w3 = Web3(WebsocketProvider(node_url))

def format_proof_nodes(proof):
    trie_proof = []
    for rlp_node in proof:
        trie_proof.append(rlp.decode(bytes(rlp_node)))
    return trie_proof

def verify_eth_get_proof(proof, root):
    trie_root = Binary.fixed_length(32, allow_empty=True)
    hash32 = Binary.fixed_length(32)
    
    for storage_proof in proof.storageProof:
        trie_key = keccak(pad_bytes(b'\x00', 32, storage_proof.key))
        root = proof.storageHash
        if storage_proof.value == b'\x00':
            rlp_value = b''
        else:
            rlp_value = rlp.encode(storage_proof.value)

        assert rlp_value == HexaryTrie.get_from_proof(
            root, trie_key, format_proof_nodes(storage_proof.proof)
        ), f"Failed to verify storage proof {storage_proof.key}"

    return True

def get_storage_proof(contract_address, storage_slot, block_number="latest"):
    contract = w3.eth.contract(address=contract_address, abi=[])  # Use your contract's ABI

     # Get state root of the specified block
    block = w3.eth.get_block(block_number)
    state_root = block['stateRoot']
   
    proof = w3.eth.get_proof(contract_address, [storage_slot], block.number)
    is_valid_proof = verify_eth_get_proof(proof, state_root)
    assert(is_valid_proof)
    return proof

def bytes_to_cpp_hardcode(buf):
    return '{' + ','.join("0x{:x}".format(x) for x in buf) + '}'

def byte_values_to_array_of_ints(val, padding_to):
    #returns something like " {'array': [ {'int': '0xf9'}, {'int': '0xe1}, ... ]
    arr = []
    i = 0
    total = len(bytes(val))
    for b in range(0, padding_to):
        if i < total:
            arr.append({'int': hex(int(bytes(val)[i]))})
        else:
            arr.append({'int': 0})
        i +=1
        
    return {'array': arr}


def int_values_to_array_of_ints(val, padding_to):
    arr = []
    i = 0
    total = len(val)
    for v in range(0, padding_to):
        if i < total:
            arr.append({'int': hex(int(val[i]))})
        else:
            arr.append({'int': 0})
        i += 1 
    return {'array': arr}


if __name__ == "__main__":
    proof = get_storage_proof(contract_address, storage_slot, block_number)
    
    block = w3.eth.get_block(block_number)
    state_root = block['stateRoot']
    
    print("Block {}".format(block_number))
    print("Contract: {}".format(contract_address))
    print("Slot: {}".format(storage_slot))
    value = proof.storageProof[0].value
    print("Value: {}".format(value.hex()))
    
    # part of data passed to inputs
    print("Proof data:")

    print("    StorageHash: {}".format(proof.storageHash.hex()))
    trie_key = keccak(pad_bytes(b'\x00', 32, proof.storageProof[0].key))
    print("    TrieKey: {}".format(trie_key.hex()))
   
    #proof_dec = proof.storageProof[0].proof[0]
    print("    Proof:")
    for p in proof.storageProof[0].proof:
        print("        {}".format(p.hex()))

    proofvs = []
    proofls = []
    i = 0
    for p in proof.storageProof[0].proof:
        proofls.append(0)
        for b in bytes(p):
            proofls[i] += 1
            proofvs.append(b)
        i += 1

    proof_vals = byte_values_to_array_of_ints(proofvs, 4096)
    proof_lengths = int_values_to_array_of_ints(proofls, 32)

    res = [
        byte_values_to_array_of_ints(proof.storageHash, 32),
        byte_values_to_array_of_ints(trie_key, 32),
        proof_lengths,
        proof_vals
    ]
    
    output_pi = "src/main-input.json"
    with open(output_pi, 'w') as f:
        sys.stdout = f
        print(json.dumps(res, indent=2))



