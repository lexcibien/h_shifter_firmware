import { Command, USB_PRODUCT_ID, USB_VENDOR_ID, WebUsbClient, decodeConfig, encodeConfig } from "./protocol.js";

const $ = (id) => document.getElementById(id);
const client = new WebUsbClient();
let info;
let config;
let pollTimer;
let unlocked = false;
let saving = false;

const colorHex = (value) => `#${value.toString(16).padStart(6, "0")}`;
const colorValue = (value) => Number.parseInt(value.slice(1), 16);
const setNotice = (message, error = false) => { $("notice").textContent = message; $("notice").style.color = error ? "#ff9b9b" : ""; };

function setSaveStatus(message, error = false) {
  $("save-status").textContent = message;
  $("save-status").classList.toggle("error", error);
}

function updateActions() {
  for (const id of ["save", "restore", "reboot", "redetect"]) $(id).disabled = !unlocked || saving;
}

async function loadConfig() {
  const bytes = await client.request(Command.GET_CONFIG);
  config = decodeConfig(bytes);
  const generation = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(20, true);
  $("source").textContent = bytes[24] ? `flash · geração ${generation}` : "padrões do firmware";
  return { bytes, generation };
}

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
  await loadConfig();
  $("project").textContent = `V${info.project}`;
  $("firmware").textContent = info.version;
  setSaveStatus("");
  $("connection").textContent = "Conectado";
  $("device-card").classList.remove("hidden");
  $("config-form").classList.remove("hidden");
  $("connect").textContent = "Desconectar";
  $("unlock-instruction").textContent = info.project === 1 ? "Segure o botão sequencial + switch direito por 1 segundo." : "Coloque em sequencial e segure o switch direito por 1 segundo.";
  renderConfig();
  await updateStatus();
  scheduleStatusPoll();
}

function scheduleStatusPoll() {
  const timer = setTimeout(async () => {
    try {
      await updateStatus();
      if (pollTimer === timer) scheduleStatusPoll();
    } catch (error) {
      if (pollTimer === timer) await disconnected(error.message, true);
    }
  }, 300);
  pollTimer = timer;
}

async function connect() {
  $("connect").disabled = true;
  try {
    if (client.device?.opened) { await disconnected(); return; }
    const device = await navigator.usb.requestDevice({ filters: [{ vendorId: USB_VENDOR_ID, productId: USB_PRODUCT_ID }] });
    await client.connect(device);
    await loadDevice();
    setNotice("Dispositivo conectado. As alterações só são gravadas ao pressionar Salvar.");
  } catch (error) { await disconnected(error.message, true); }
  finally { $("connect").disabled = false; }
}

async function disconnected(message = "Dispositivo desconectado.", error = false) {
  clearTimeout(pollTimer);
  pollTimer = undefined;
  await client.close().catch(() => {});
  unlocked = false;
  updateActions();
  $("connection").textContent = "Desconectado";
  $("device-card").classList.add("hidden");
  $("config-form").classList.add("hidden");
  $("connect").textContent = "Conectar dispositivo";
  setNotice(message, error);
}

async function updateStatus() {
  const bytes = await client.request(Command.GET_STATUS);
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  unlocked = (bytes[0] & 1) !== 0;
  $("lock-help").classList.toggle("hidden", unlocked);
  updateActions();
  $("mode").textContent = bytes[0] & 2 ? "Sequencial" : "H";
  $("handle").textContent = bytes[0] & 4 ? "Conectada" : "Não conectada";
  $("adc").textContent = view.getUint16(1, true);
  $("inputs").textContent = `0x${view.getUint16(3, true).toString(16).padStart(4, "0")}`;
  $("hid").textContent = `0x${view.getUint16(5, true).toString(16).padStart(4, "0")}`;
}

$("connect").addEventListener("click", connect);
$("config-form").addEventListener("input", () => {
  updateOutputs();
  $("dirty").classList.remove("hidden");
  if (!saving) setSaveStatus("");
});
$("config-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  if (saving || !unlocked) return;
  saving = true;
  updateActions();
  setSaveStatus("Salvando na flash…");
  const requested = encodeConfig(readForm());
  try {
    const result = await client.request(Command.SAVE_CONFIG, requested);
    const savedGeneration = new DataView(result.buffer, result.byteOffset, result.byteLength).getUint32(0, true);
    const { bytes, generation } = await loadConfig();
    if (bytes[24] !== 1 || generation !== savedGeneration || !requested.every((byte, index) => byte === bytes[index])) {
      throw new Error("A configuração lida não corresponde à solicitada.");
    }
    const currentForm = encodeConfig(readForm());
    if (currentForm.every((byte, index) => byte === requested[index])) renderConfig();
    setSaveStatus(`Configuração salva na flash (geração ${generation}).`);
  } catch (error) {
    const message = `Não foi possível confirmar a gravação. ${error.message}`;
    setSaveStatus(message, true);
    if (!client.device?.opened) await disconnected(message, true);
  } finally {
    saving = false;
    updateActions();
  }
});
$("redetect").addEventListener("click", async () => { try { await client.request(Command.REDETECT_HANDLE); await updateStatus(); } catch (error) { setNotice(error.message, true); } });
$("restore").addEventListener("click", async () => { if (!confirm("Restaurar e gravar todos os padrões de fábrica?")) return; try { await client.request(Command.RESTORE_DEFAULTS); await loadConfig(); renderConfig(); setSaveStatus(""); setNotice("Padrões restaurados."); } catch (error) { setNotice(error.message, true); } });
$("reboot").addEventListener("click", async () => { if (!confirm("Reiniciar o dispositivo agora?")) return; try { await client.request(Command.REBOOT); setNotice("Reiniciando; reconecte quando o dispositivo voltar."); } catch (error) { setNotice(error.message, true); } });

if (!("usb" in navigator)) { $("connect").disabled = true; setNotice("Este navegador não oferece WebUSB. Use Chrome ou Edge em HTTPS.", true); }
else { navigator.usb.addEventListener("disconnect", ({ device }) => { if (device === client.device) disconnected(); }); }
