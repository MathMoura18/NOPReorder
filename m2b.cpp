#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <bitset>

using namespace std;

class RISCVInstructionReader {
private:
    struct InstructionInfo {
        string originalHex;
        string binary;
        string type;
    };

    static vector<InstructionInfo> allInstructions;

    int total = 0;
    int alu = 0;
    int jump = 0;
    int branch = 0;
    int memory = 0;
    int other = 0;
    int nopCountWithoutForwarding = 0;
    int nopCountWithForwarding = 0;

    const unordered_set<string> ALU_OPCODES = {"0110011", "0010011"};
    const unordered_set<string> JUMP_OPCODES = {"1101111", "1100111"};
    const unordered_set<string> BRANCH_OPCODES = {"1100011"};
    const unordered_set<string> MEMORY_OPCODES = {"0000011", "0100011"};

    vector<string> instructions;
    vector<string> modifiedInstructionsWithForwarding;

    string hexToBinary(const string& hex) const {
        unsigned int n = stoul(hex, nullptr, 16);
        bitset<32> binary(n);
        return binary.to_string();
    }
    
    string classifyInstruction(const string& binary) {
        string opcode = binary.substr(25, 7);
        if (ALU_OPCODES.count(opcode)) {
            alu++;
            return "ALU";
        } else if (JUMP_OPCODES.count(opcode)) {
            jump++;
            return "Jump";
        } else if (BRANCH_OPCODES.count(opcode)) {
            branch++;
            return "Branch";
        } else if (MEMORY_OPCODES.count(opcode)) {
            memory++;
            return "Memory";
        } else if (opcode == "00000013"){
            return "NOP";
        } else {
            other++;
            return "Other";
        }
    }

    void classifyAndDetectConflicts() {
        string previousInstruction;
        for (size_t i = 0; i < instructions.size(); i++) {
            string instruction = instructions[i];
            string binary = hexToBinary(instruction);
            string type = classifyInstruction(binary);

            allInstructions.push_back({instruction, binary, type});

            total++;

            if (!previousInstruction.empty()) {
                if (detectDataHazard(previousInstruction, binary, type)) {
                    cout << "Conflito detectado entre instrucao " << (i - 1) << " e instrucao " << i << endl;
                    insertNOPsWithForwarding();
                }
            }

            previousInstruction = binary;

            modifiedInstructionsWithForwarding.push_back(instruction);
        }
    }

    bool detectDataHazard(const string& previous, const string& current, string instructionType) {
        bool isInstructionMemory = false;

        if (instructionType == "Memory")
            isInstructionMemory = true;

        string prevDestReg = previous.substr(20, 5);
        string currSrcReg1 = current.substr(12, 5);
        if (isInstructionMemory) {
            return prevDestReg == currSrcReg1;
        } else {
            string currSrcReg2 = current.substr(7, 5);
            return prevDestReg == currSrcReg1 || prevDestReg == currSrcReg2;
        }
    }

    void insertNOPsWithForwarding() {
        modifiedInstructionsWithForwarding.push_back("00000013");  // NOP
        nopCountWithForwarding++;
    }

    void applyDelayedBranch() {
        string previousInstruction;
        for (size_t i = 0; i < modifiedInstructionsWithForwarding.size(); ++i) {
            string instruction = modifiedInstructionsWithForwarding[i];
            string binary = hexToBinary(instruction);
            string type = classifyInstruction(binary);
            string prevDestReg = binary.substr(20, 5);

            if (!previousInstruction.empty()){
                if (detectDataHazard(previousInstruction, binary, type) || type == "NOP"){
                    string previousInstructionCounter;
                    for (int counter = i; counter >= 0; counter--) {
                        string instructionCounter = modifiedInstructionsWithForwarding[counter];
                        string binaryCounter = hexToBinary(instructionCounter);
                        string typeCounter = classifyInstruction(binaryCounter);

                        if (!previousInstructionCounter.empty()) {
                            string prevDestRegCounter = binaryCounter.substr(20, 5);
                            string currSrcReg1Counter = binaryCounter.substr(12, 5);
                            string currSrcReg2Counter = binaryCounter.substr(7, 5);

                            if (prevDestReg == prevDestRegCounter || prevDestReg == currSrcReg1Counter || prevDestReg == currSrcReg2Counter) {
                                modifiedInstructionsWithForwarding.insert(modifiedInstructionsWithForwarding.begin() + i + 1, "00000013");
                            }
                            else {
                                modifiedInstructionsWithForwarding.insert(modifiedInstructionsWithForwarding.begin() + (i + 1), instructionCounter);
                                modifiedInstructionsWithForwarding[counter] = "";
                            }
                        }
                        previousInstructionCounter = binaryCounter;
                    }
                }
            }
            previousInstruction = binary;
        }
    }

public:
    void processFile(const string& fileName) {
        ifstream file(fileName);
        if (!file.is_open()) {
            cerr << "Erro ao abrir o arquivo: " << fileName << endl;
            return;
        }
        string line;
        while (getline(file, line)) {
            instructions.push_back(line);
        }
        file.close();
        classifyAndDetectConflicts();
        applyDelayedBranch();
    }

    void printInitialInstructions() {
        cout << "Lista Inicial de Instru��es:" << endl;
        for (size_t i = 0; i < instructions.size(); ++i) {
            string binary = hexToBinary(instructions[i]);
            string type = classifyInstruction(binary);
            cout << i + 1 << ". Hex: " << instructions[i] << " - Tipo: " << type << endl;
        }
        cout << endl;
    }

    void printModifiedInstructions() {
        cout << "Lista Modificada de Instru��es (com NOPs e/ou reordenamento):" << endl;
        for (size_t i = 0; i < modifiedInstructionsWithForwarding.size(); ++i) {
            string instruction = modifiedInstructionsWithForwarding[i];
            if (instruction == "00000013") {
                cout << i + 1 << ". NOP inserido" << endl;
            } else {
                cout << i + 1 << ". Hex: " << instruction << " - Tipo: ";
                string binary = hexToBinary(instruction);
                cout << classifyInstruction(binary) << endl;
            }
        }
        cout << endl;
    }

    void printStatistics() {
        cout << "Total de Instrucoes: " << total << endl;
        cout << "Instrucoes ALU: " << alu << endl;
        cout << "Instrucoes Jump: " << jump << endl;
        cout << "Instrucoes Branch: " << branch << endl;
        cout << "Instrucoes Memory: " << memory << endl;
        cout << "Outras Instrucoes: " << other << endl;
        cout << "NOPs inseridos com forwarding: " << nopCountWithForwarding << endl;
    }

    // M�todo de retorno das instru��es modificadas com forwarding
    const vector<string>& getModifiedInstructionsWithForwarding() {
        return modifiedInstructionsWithForwarding;
    }

    void generateModifiedFile(const string& fileName, const vector<string>& modifiedInstructions) {
        ofstream file(fileName);
        if (!file.is_open()) {
            cerr << "Erro ao gerar o arquivo: " << fileName << endl;
            return;
        }
        for (const auto& instruction : modifiedInstructions) {
            file << instruction << endl;
        }
        file.close();
    }
};

vector<RISCVInstructionReader::InstructionInfo> RISCVInstructionReader::allInstructions;

int main(int argc, char* argv[]) {
    string filePath;

    // if (argc == 2) {
    //     filePath = argv[1];
    // } else {
    //     cout << "Digite o caminho do arquivo hex: ";
    //     getline(cin, filePath);
    // }

    filePath = "C:\\Users\\8244308\\Downloads\\exemplohex.hex";
    RISCVInstructionReader classifier;
    classifier.processFile(filePath);

    classifier.printInitialInstructions();
    classifier.printStatistics();
    classifier.printModifiedInstructions();

    classifier.generateModifiedFile("modified_with_forwarding.hex", classifier.getModifiedInstructionsWithForwarding());
    return 0;
}