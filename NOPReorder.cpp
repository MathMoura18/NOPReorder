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
    vector<string> modifiedInstructionsWithReordering;

    string hexToBinary(const string& hex) const {
        unsigned int n = stoul(hex, nullptr, 16);
        bitset<32> binary(n);
        return binary.to_string();
    }
    
    string classifyInstruction(const string& binary) {
        string opcode = binary.substr(25, 7);
        if (binary == "00000000000000000000000000010011"){
            return "NOP";
        } else if (ALU_OPCODES.count(opcode)) {
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

            if (type == "Jump")
                insertNOPsWithForwarding();
        }
    }

    bool detectDataHazard(const string& previous, const string& current, string currentInstructionType) {
        bool isInstructionMemory = false;

        if (currentInstructionType == "Memory")
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
        int flag = 0;
        for (size_t i = 0; i < modifiedInstructionsWithForwarding.size(); ++i) {
            string instruction = modifiedInstructionsWithForwarding[i];
            string binary = hexToBinary(instruction);
            string type = classifyInstruction(binary);
            string prevDestReg = binary.substr(20, 5);

            bool isNOPNecessary = false;
            if (!previousInstruction.empty()){
                if (detectDataHazard(previousInstruction, binary, type) || type == "NOP"){                
                    // Se estiver analisando um NOP, então deve-se analisar a instrução anterior a ele
                    if (type == "NOP"){
                        string prevNOPInstruction = modifiedInstructionsWithForwarding[i - 1];
                        string prevNOPbinary = hexToBinary(prevNOPInstruction);
                        string prevNOPType = classifyInstruction(prevNOPbinary);
                        string prevNOPDestReg = prevNOPbinary.substr(20, 5);
                        prevDestReg = prevNOPDestReg;
                        for (int x = 2; prevNOPType == "NOP"; x++){
                            prevNOPInstruction = modifiedInstructionsWithForwarding[i - x];
                            prevNOPbinary = hexToBinary(prevNOPInstruction);
                            prevNOPType = classifyInstruction(prevNOPbinary);
                            prevNOPDestReg = prevNOPbinary.substr(20, 5);
                            prevDestReg = prevNOPDestReg;
                        }
                    }

                    for (int counter = i - 1; counter > flag; counter--) {
                        string instructionCounter = modifiedInstructionsWithForwarding[counter];
                        string binaryCounter = hexToBinary(instructionCounter);
                        string typeCounter = classifyInstruction(binaryCounter);

                        string prevDestRegCounter = binaryCounter.substr(20, 5);
                        string prevSrcReg1Counter = binaryCounter.substr(12, 5);
                        string prevSrcReg2Counter = binaryCounter.substr(7, 5);
                        string currSrcReg1Counter = binaryCounter.substr(12, 5);
                        string currSrcReg2Counter = binaryCounter.substr(7, 5);

                        // Condições para reordenação
                        if ((typeCounter != "NOP") && (prevDestReg != prevDestRegCounter) && (prevDestReg != currSrcReg1Counter) && (prevDestReg != currSrcReg2Counter)) {
                            modifiedInstructionsWithReordering.push_back(instructionCounter);
                            modifiedInstructionsWithReordering[counter] = "";
                            isNOPNecessary = false;  
                            flag = i;
                            break;                          
                        }
                        isNOPNecessary = true;
                    }
                }
            }
            previousInstruction = binary;
            
            if (isNOPNecessary){
                if (type == "NOP") {
                    modifiedInstructionsWithReordering.push_back(instruction);
                } else {
                    modifiedInstructionsWithReordering.push_back("00000013");
                    modifiedInstructionsWithReordering.push_back(instruction);
                }
            } else {
                if (type != "NOP"){
                    modifiedInstructionsWithReordering.push_back(instruction);
                }
            }
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

    void printModifiedInstructions() {
        cout << "Lista Modificada de Instrucoes (com NOPs):" << endl;
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

    void printReorderedInstructions() {
        cout << "Lista Reordenada de Instrucoes (com NOPs e/ou reordenamento):" << endl;
        for (size_t i = 0; i < modifiedInstructionsWithReordering.size(); ++i) {
            string instruction = modifiedInstructionsWithReordering[i];
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

    const vector<string>& getModifiedInstructionsWithReordering() {
        return modifiedInstructionsWithReordering;
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

    filePath = "C:\\dev\\wsEducational\\univali\\NOPReorder\\example.hex";

    RISCVInstructionReader classifier;
    classifier.processFile(filePath);

    classifier.printStatistics();
    classifier.printModifiedInstructions();

    classifier.generateModifiedFile("modified_with_forwarding.hex", classifier.getModifiedInstructionsWithReordering());
    return 0;
}