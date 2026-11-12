import path from "path";
import os from "os";
import { execFile } from "child_process";
import { promisify } from "util";

export type BinFile = {
    filePath: string,
    delete: boolean
}

export async function getBinaryFile(filePath: string): Promise<BinFile> {
    const fileType = path.extname(filePath).toLowerCase();
    switch (fileType) {
        case ".bin":
            return {
                filePath: filePath,
                delete: false,
            };
        case ".elf":
            const binPath = path.join(os.tmpdir(), "wireless-upload-" + path.basename(filePath, ".elf") + ".bin");

            try {
                await promisify(execFile)("arm-none-eabi-objcopy", [
                    "-O",
                    "binary",
                    filePath,
                    binPath
                ]);
            } catch (error) {
                throw new Error("Could not convert .elf to .bin, `arm-none-eabi-objcopy` is probably missing");
            }

            return {
                filePath: binPath,
                delete: true
            };
        default:
            throw new Error("Unsupported file type: " + fileType);
    }
}