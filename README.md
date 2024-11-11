# NOPReorder

**NOPReorder** is a project developed to reorder and insert NOP (No Operation) instructions into RISC-V Assembly code, aiming to optimize execution, fix alignment issues, or improve the overall structure of the code. This process is useful in reverse engineering, performance analysis, or embedded systems that require precise control over code execution.

### Features

- **Instruction Reordering**: Reorders assembly code instructions for more efficient execution, considering alignment and flow.
- **NOP Insertion**: Inserts `NOP` instructions to optimize code execution or fix alignment issues.
- **Code Analysis and Modification**: Reads and modifies Assembly code with intelligent NOP insertion, without affecting the final execution result.

### How It Works

1. **Reading the Assembly Code**: The project starts by reading an RISC-V Assembly code file in hexadecimal format.
2. **Instruction Analysis**: The code is analyzed to find reordering points or alignment issues.
3. **Reordering and NOP Insertion**: The algorithm reorders instructions and inserts the necessary NOPs to ensure the code is executed optimally and efficiently.
4. **Output of Modified Code**: The modified code is then generated, ready to be used in its original application.

### Usage

To use this project, you only need to download it and run it!
1. Clone the repository:

   CMD: git clone https://github.com/MathMoura18/NOPReorder.git

2. Execute NOPReader:
   
   Run the file: NOPReorder.exe

### Contributing

Contributions are welcome! If you have suggestions or improvements, feel free to open an issue or submit a pull request.

#### How to contribute:

1. Fork this repository.
2. Create a branch for your feature (git checkout -b my-feature).
3. Commit your changes (git commit -am 'Adding new feature').
4. Push to your fork (git push origin my-feature).
5. Open a pull request.
