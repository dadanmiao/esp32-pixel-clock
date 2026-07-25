import { copyFile, mkdir } from "node:fs/promises";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const scriptDir = dirname(fileURLToPath(import.meta.url));
const appDir = dirname(scriptDir);
const webDir = join(appDir, "www");
const webAssets = [
  "index.html",
  "styles.css",
  "app.js",
  "manifest.webmanifest",
];

await mkdir(webDir, { recursive: true });

for (const asset of webAssets) {
  await copyFile(join(appDir, asset), join(webDir, asset));
}

console.log(`[mobile] Copied ${webAssets.length} web assets to ${webDir}`);
