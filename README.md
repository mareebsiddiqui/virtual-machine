# AreebOS (Virtual Machine)
A light weight virtual machine written in C++ with a fetch-execute cycle visualizer.
#### Inspired from: [https://github.com/justinmeiners/lc3-vm](https://github.com/justinmeiners/lc3-vm)

## How to use:
`./areebos object_file.obj`

## Components:
1. `areebos`: The virtual machine that takes in an object file as input
2. `visualizer`: The desktop app that visualizes the fetch execute cycle

## Instruction set:
| Instruction | Syntax                                       |
| :---------: | -------------------------------------------- |
| ADD         | `ADD dest_reg, operand_reg_1, operand_reg_2` |