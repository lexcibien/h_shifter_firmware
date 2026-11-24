import { Command, USB_PRODUCT_ID, USB_VENDOR_ID, WebUsbClient, decodeConfig, encodeConfig } from "./protocol.js";

const $ = (id) => document.getElementById(id);
const client = new WebUsbClient();
let info;
let config;
let pollTimer;

const colorHex = (value) => `#${value.toString(16).padStart(6, "0")}`;
const colorValue = (value) => Number.parseInt(value.slice(1), 16);
const setNotice = (message, error = false) => { $("notice").textContent = message; $("notice").style.color = error ? "#ff9b9b" : ""; };

function renderConfig() {
  $("gears").max = info.maxGears;
  $("gears").value = config.forwardGearCount;
  $("default-mode").checked = config.defaultSequentialMode;
  $("reverse").checked = config.reverseEnabled;
  $("debounce").value = config.debounceMs;
  $("hold").value = config.sequentialHoldMs;
  $("threshold").value = config.handleDetectThreshold;
  $("h-color").value = colorHex(config.hModeColor);
  $("seq-color").value = colorHex(config.sequentialModeColor);
  $("default-mode-row").classList.toggle("hidden", !(info.capabilities & 1));
  $("hold-row").classList.toggle("hidden", !(info.capabilities & 2));
  updateOutputs();
  $("dirty").classList.add("hidden");
}

function readForm() {
  return {
    forwardGearCount: Number($("gears").value),
    defaultSequentialMode: $("default-mode").checked,
    reverseEnabled: $("reverse").checked,
    debounceMs: Number($("debounce").value),
    sequentialHoldMs: Number($("hold").value),
    handleDetectThreshold: Number($("threshold").value),
    hModeColor: colorValue($("h-color").value),
    sequentialModeColor: colorValue($("seq-color").value),
  };
}

function updateOutputs() {
  $("gear-output").value = `${$("gears").value} marchas`;
  $("debounce-output").value = `${$("debounce").value} ms`;
  $("hold-output").value = `${$("hold").value} ms`;
  $("threshold-output").value = $("threshold").value;
}

async function loadDevice() {
  const infoBytes = await client.request(Command.GET_INFO);
  const infoView = new DataView(infoBytes.buffer, infoBytes.byteOffset, infoBytes.byteLength);
  info = { schema: infoBytes[0], project: infoBytes[1], maxGears: infoBytes[2], capabilities: infoView.getUint16(3, true), version: `${infoBytes[5]}.${infoBytes[6]}.${infoBytes[7]}` };
  const configBytes = await client.request(Command.GET_CONFIG);
  config = decodeConfig(configBytes);
  const configView = new DataView(configBytes.buffer, configBytes.byteOffset, configBytes.byteLength);
  const generation = configView.getUint32(20, true);
  $("project").textContent = `V${info.project}`;
  $("firmware").textContent = info.version;
  $("source").textContent = configBytes[24] ? `flash · geração ${generation}` : "padrões do firmware";
  $("connection").textContent = "Conectado";
  $("device-card").classList.remove("hidden");
  $("config-form").classList.remove("hidden");
  $("connect").textContent = "Desconectar";
  $("unlock-instruction").textContent = info.project === 1 ? "Segure o botão sequencial + switch direito por 1 segundo." : "Coloque em sequencial e segure o switch direito por 1 segundo.";
  renderConfig();
  await updateStatus();
  pollTimer = setInterval(() => updateStatus().catch(disconnected), 300);
}

async function connect() {
  try {
    if (client.device?.opened) { await disconnected(); return; }
    const device = await navigator.usb.requestDevice({ filters: [{ vendorId: USB_VENDOR_ID, productId: USB_PRODUCT_ID }] });
    await client.connect(device);
    await loadDevice();
    setNotice("Dispositivo conectado. As alterações só são gravadas ao pressionar Salvar.");
  } catch (error) { setNotice(error.message, true); }
}

async function disconnected() {
  clearInterval(pollTimer);
  await client.close().catch(() => {});
  $("device-card").classList.add("hidden");
  $("config-form").classList.add("hidden");
  $("connect").textContent = "Conectar dispositivo";
  setNotice("Dispositivo desconectado.");
}

async function updateStatus() {
  const bytes = await client.request(Command.GET_STATUS);
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const unlocked = (bytes[0] & 1) !== 0;
  $("lock-help").classList.toggle("hidden", unlocked);
  for (const id of ["save", "restore", "reboot", "redetect"]) $(id).disabled = !unlocked;
  $("mode").textContent = bytes[0] & 2 ? "Sequencial" : "H";
  $("handle").textContent = bytes[0] & 4 ? "Conectada" : "Não conectada";
  $("adc").textContent = view.getUint16(1, true);
  $("inputs").textContent = `0x${view.getUint16(3, true).toString(16).padStart(4, "0")}`;
  $("hid").textContent = `0x${view.getUint16(5, true).toString(16).padStart(4, "0")}`;
}

$("connect").addEventListener("click", connect);
$("config-form").addEventListener("input", () => { updateOutputs(); $("dirty").classList.remove("hidden"); });
$("config-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  try { await client.request(Command.SAVE_CONFIG, encodeConfig(readForm())); config = readForm(); renderConfig(); setNotice("Configuração validada e salva na flash."); }
  catch (error) { setNotice(error.message, true); }
});
$("redetect").addEventListener("click", async () => { try { await client.request(Command.REDETECT_HANDLE); await updateStatus(); } catch (error) { setNotice(error.message, true); } });
$("restore").addEventListener("click", async () => { if (!confirm("Restaurar e gravar todos os padrões de fábrica?")) return; try { await client.request(Command.RESTORE_DEFAULTS); const bytes = await client.request(Command.GET_CONFIG); config = decodeConfig(bytes); renderConfig(); setNotice("Padrões restaurados."); } catch (error) { setNotice(error.message, true); } });
$("reboot").addEventListener("click", async () => { if (!confirm("Reiniciar o dispositivo agora?")) return; try { await client.request(Command.REBOOT); setNotice("Reiniciando; reconecte quando o dispositivo voltar."); } catch (error) { setNotice(error.message, true); } });

if (!("usb" in navigator)) { $("connect").disabled = true; setNotice("Este navegador não oferece WebUSB. Use Chrome ou Edge em HTTPS.", true); }
else { navigator.usb.addEventListener("disconnect", ({ device }) => { if (device === client.device) disconnected(); }); }
