# ETH GLobal Hackathon Istanbul project

This is the repo for hackathon project by MixBytes team of ETHGLobal Hackathon Istanbul, implementing Merkle Patricia Tree storage proofs

The project is based on [ZKLLVM](https://github.com/NilFoundation/zkLLVM) compiler-assigner-prover and forked from [zkllvm-template](https://github.com/NilFoundation/zkllvm-template) project.


# Instuctions

## Cloning and updateing submodules

```bash
git clone --recurse-submodules https://github.com/mixbytes/zkllvm-mpt-proofs.git
cd zkllvm-mpt-proofs
```

If you initially cloned without `--recurse-submodules`, update submodules explicitly:

```bash
git submodule update --init --recursive
```

Prepare Docker image containing the `=nil;` toolchain:

```bash
docker pull ghcr.io/nilfoundation/toolchain:latest
```

## 1. Compile circuit
Run the script from the root of your project.

```bash
scripts/run.sh --docker compile
```

The `compile` command does the following:

1. Starts a Docker container based on `nilfoundation/zkllvm-template`. 
2. Makes a clean `./build` directory and initializes `cmake`.
3. Compiles the code into a circuit.


## 2. Build a circuit file and an assignment table (analog of witness generation)

Next step is to make a compiled circuit and assignment table.

```bash
scripts/run.sh --docker run_assigner
```

On this step, we run the `assigner`, giving it the circuit in LLVM IR format (`template.lls`)
and the input data (`./src/main-input.json`).
The `assigner` produces the following files:

* Circuit file `./build/template.crct` is the circuit in a binary format that is
  usable by the `proof-generator`.
* Assignment table `./build/template.tbl` is a representation of input data,
  prepared for proof computation with this particular circuit.

## 3. Produce and verify a proof locally

```bash
scripts/run.sh --docker prove
```


## 4. Do something

Study ZKLLVM repos for more information, and use MPT proofs for building more secure & trustless oracles. Good luck!


# (to be continued) 
