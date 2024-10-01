#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <bitset>

using namespace std;

// Contadores de instruções e sobrecusto
int total = 0;
int alu = 0;
int jump = 0;
int branch = 0;
int memory = 0;
int other = 0;
int nopCountWithoutForwarding = 0;
int nopCountWithForwarding = 0;

// Mapeamento de opcodes para os tipos de instruções
const unordered_set<string> ALU_OPCODES = {"0110011", "0010011"};
const unordered_set<string> JUMP_OPCODES = {"1101111", "1100111"};
const unordered_set<string> BRANCH_OPCODES = {"1100011"};
const unordered_set<string> MEMORY_OPCODES = {"0000011", "0100011"};

// Armazenar instruções para processar conflitos
vector<string> instructions;
vector<string> modifiedInstructionsWithoutForwarding;
vector<string> modifiedInstructionsWithForwarding;

// Função para converter hexadecimal para binário
string hexToBinary(const string& hex) {
    return bitset<32>(stoi(hex, nullptr, 16)).to_string();
}

// Função para detectar conflitos de dados
bool detectDataHazard(const string& previous, const string& current) {
    string prevDestReg = previous.substr(20, 5); // Registrador destino da instrução anterior
    string currSrcReg1 = current.substr(12, 5); // Registrador origem 1 da instrução atual
    string currSrcReg2 = current.substr(7, 5);  // Registrador origem 2 da instrução atual

    return prevDestReg == currSrcReg1 || prevDestReg == currSrcReg2;
}

// Inserir NOPs sem forwarding
void insertNOPsWithoutForwarding() {
    modifiedInstructionsWithoutForwarding.push_back("00000013"); // hexadecimal addi x0, x0, 0
    modifiedInstructionsWithoutForwarding.push_back("00000013");
    nopCountWithoutForwarding++;
}

// Inserir NOPs com forwarding (menor quantidade de NOPs)
void insertNOPsWithForwarding() {
    modifiedInstructionsWithForwarding.push_back("00000013"); // hexadecimal addi x0, x0, 0
    nopCountWithForwarding++;
}

// Classificação de instruções e detecção de dependências
void classifyAndDetectConflicts() {
    string previousInstruction;
    for (size_t i = 0; i < instructions.size(); i++) {
        string instruction = instructions[i];
        string binary = hexToBinary(instruction);
        string opcode = binary.substr(25, 7); // Os últimos 7 bits representam o opcode no RISC-V
        total++;

        // Verificar o tipo de instrução
        if (ALU_OPCODES.find(opcode) != ALU_OPCODES.end()) {
            alu++;
        } else if (JUMP_OPCODES.find(opcode) != JUMP_OPCODES.end()) {
            jump++;
        } else if (BRANCH_OPCODES.find(opcode) != BRANCH_OPCODES.end()) {
            branch++;
        } else if (MEMORY_OPCODES.find(opcode) != MEMORY_OPCODES.end()) {
            memory++;
        } else {
            other++;
        }

        // Identificação de conflitos de dados com a instrução anterior
        if (!previousInstruction.empty()) {
            if (detectDataHazard(previousInstruction, instruction)) {
                // Exibir o local do conflito
                cout << "Conflito detectado entre instrução " << (i - 1) << " e instrução " << i << endl;

                // Inserir NOPs sem forwarding
                insertNOPsWithoutForwarding();
                // Inserir NOPs com forwarding (apenas se necessário)
                insertNOPsWithForwarding();
            }
        }

        // Armazenar a instrução atual como a anterior para a próxima iteração
        previousInstruction = instruction;

        // Adicionar instrução modificada
        modifiedInstructionsWithoutForwarding.push_back(instruction);
        modifiedInstructionsWithForwarding.push_back(instruction);
    }
}

// Função para ler o arquivo de entrada
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
}

// Função para gerar arquivo com instruções modificadas
void generateModifiedFile(const string& fileName, const vector<string>& modifiedInstructions) {
    ofstream file(fileName);
    if (!file.is_open()) {
        cerr << "Erro ao gerar o arquivo: " << fileName << endl;
        return;
    }

    for (const string& instruction : modifiedInstructions) {
        file << instruction << endl;
    }

    file.close();
}

// Exibe as estatísticas
void printStatistics() {
    cout << "Total de Instruções: " << total << endl;
    cout << "Instruções ALU: " << alu << endl;
    cout << "Instruções Jump: " << jump << endl;
    cout << "Instruções Branch: " << branch << endl;
    cout << "Instruções Memory: " << memory << endl;
    cout << "Outras Instruções: " << other << endl;
    cout << "NOPs inseridos sem forwarding: " << nopCountWithoutForwarding << endl;
    cout << "NOPs inseridos com forwarding: " << nopCountWithForwarding << endl;
}

// Função principal
int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Uso: ./RISCVInstructionReader <arquivo_hex>" << endl;
        return 1;
    }

    string fileName = argv[1];
    processFile(fileName);
    printStatistics();

    // Gera arquivos com as instruções modificadas
    generateModifiedFile("modified_without_forwarding.hex", modifiedInstructionsWithoutForwarding);
    generateModifiedFile("modified_with_forwarding.hex", modifiedInstructionsWithForwarding);

    return 0;
}