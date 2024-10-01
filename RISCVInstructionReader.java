import java.io.*;
import java.util.*;

public class RISCVInstructionReader {

    private int total = 0;
    private int alu = 0;
    private int jump = 0;
    private int branch = 0;
    private int memory = 0;
    private int other = 0;
    private int nopCountWithoutForwarding = 0;
    private int nopCountWithForwarding = 0;

    private static final Set<String> ALU_OPCODES = new HashSet<>(Arrays.asList("0110011", "0010011"));
    private static final Set<String> JUMP_OPCODES = new HashSet<>(Arrays.asList("1101111", "1100111"));
    private static final Set<String> BRANCH_OPCODES = new HashSet<>(Arrays.asList("1100011"));
    private static final Set<String> MEMORY_OPCODES = new HashSet<>(Arrays.asList("0000011", "0100011"));

    private List<String> instructions = new ArrayList<>();
    private List<String> modifiedInstructionsWithoutForwarding = new ArrayList<>();
    private List<String> modifiedInstructionsWithForwarding = new ArrayList<>();

    private String hexToBinary(String hex) {
        return String.format("%32s", Integer.toBinaryString(Integer.parseUnsignedInt(hex, 16))).replace(' ', '0');
    }

    private void classifyAndDetectConflicts() {
        String previousInstruction = null;
        for (int i = 0; i < instructions.size(); i++) {
            String instruction = instructions.get(i);
            String binary = hexToBinary(instruction);
            String opcode = binary.substring(25, 32); // Os últimos 7 bits representam o opcode no RISC-V
            total++;

            if (ALU_OPCODES.contains(opcode)) {
                alu++;
            } else if (JUMP_OPCODES.contains(opcode)) {
                jump++;
            } else if (BRANCH_OPCODES.contains(opcode)) {
                branch++;
            } else if (MEMORY_OPCODES.contains(opcode)) {
                memory++;
            } else {
                other++;
            }

            if (previousInstruction != null) {
                if (detectDataHazard(previousInstruction, binary)) {
                    System.out.println("Conflito detectado entre instrucao " + (i - 1) + " e instrucao " + i);
                    insertNOPsWithoutForwarding();
                    insertNOPsWithForwarding();
                }
            }

            previousInstruction = binary;

            modifiedInstructionsWithoutForwarding.add(instruction);
            modifiedInstructionsWithForwarding.add(instruction);
        }
    }

    private boolean detectDataHazard(String previous, String current) {
        String prevDestReg = previous.substring(20, 25); // Registrador destino da instrução anterior
        String currSrcReg1 = current.substring(12, 17); // Registrador origem 1 da instrução atual
        String currSrcReg2 = current.substring(7, 12);  // Registrador origem 2 da instrução atual

        return prevDestReg.equals(currSrcReg1) || prevDestReg.equals(currSrcReg2);
    }

    private void insertNOPsWithoutForwarding() {
        modifiedInstructionsWithoutForwarding.add("00000013"); // hexadecimal addi x0, x0, 0
        nopCountWithoutForwarding++;
        modifiedInstructionsWithoutForwarding.add("00000013");
        nopCountWithoutForwarding++;
        //Sem forwading é inseridos mais NOPs que o necessário
    }

    private void insertNOPsWithForwarding() {
        modifiedInstructionsWithForwarding.add("00000013"); // hexadecimal addi x0, x0, 0
        nopCountWithForwarding++;
        //Com forwading é inseridos apenas os NOPs necessários
    }

    public void processFile(String fileName) {
        try (BufferedReader br = new BufferedReader(new FileReader(fileName))) {
            String line;
            while ((line = br.readLine()) != null) {
                instructions.add(line.trim());
            }
            classifyAndDetectConflicts();
        } catch (IOException e) {
            System.out.println("Erro ao ler o arquivo: " + e.getMessage());
        }
    }

    private void generateModifiedFile(String fileName, List<String> modifiedInstructions) {
        try (BufferedWriter writer = new BufferedWriter(new FileWriter(fileName))) {
            for (String instruction : modifiedInstructions) {
                writer.write(instruction);
                writer.newLine();
            }
        } catch (IOException e) {
            System.out.println("Erro ao gerar o arquivo: " + e.getMessage());
        }
    }

    // Exibe as estatísticas
    public void printStatistics() {
        System.out.println("Total de Instrucoes: " + total);
        System.out.println("Instrucoes ALU: " + alu);
        System.out.println("Instrucoes Jump: " + jump);
        System.out.println("Instrucoes Branch: " + branch);
        System.out.println("Instrucoes Memory: " + memory);
        System.out.println("Outras Instrucoes: " + other);
        System.out.println("NOPs inseridos sem forwarding: " + nopCountWithoutForwarding);
        System.out.println("NOPs inseridos com forwarding: " + nopCountWithForwarding);
    }

    public static void main(String[] args) throws FileNotFoundException {
        try {
            // if (args.length != 1)
                // throw new FileNotFoundException("Tente novamente: java RiscVInstructionClassifier <arquivo_hex>");

            RISCVInstructionReader classifier = new RISCVInstructionReader();
            // classifier.processFile(args[0]);
            classifier.processFile("teste.hex");
            classifier.printStatistics();

            // Gera arquivos com as instruções modificadas
            classifier.generateModifiedFile("modified_without_forwarding.hex", classifier.modifiedInstructionsWithoutForwarding);
            classifier.generateModifiedFile("modified_with_forwarding.hex", classifier.modifiedInstructionsWithForwarding);
        }
        catch (Exception ex) {
            throw ex;
        }
    }
}
