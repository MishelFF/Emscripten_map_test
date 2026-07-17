let Module = null;
let wasmLoaded = false;

const dropArea = document.getElementById('drop-area');
const fileInput = document.getElementById('file-input');
const btnSelect = document.getElementById('btn-select');
const btnClear = document.getElementById('btn-clear');
const fileInfo = document.getElementById('file-info');
const fileName = document.getElementById('file-name');
const fileSize = document.getElementById('file-size');
const statusText = document.getElementById('status-text');

// Загрузка WASM
async function loadWASM() {
    try {
        statusText.textContent = 'Загрузка WASM...';
        Module = await FileVisualizer({ locateFile: (path) => `./${MapTest.wasm}` });
        wasmLoaded = true;
        statusText.textContent = 'Готов';
        console.log('✅ WASM загружен, функции:', Object.keys(Module).filter(k => typeof Module[k] === 'function'));
    } catch (e) {
        console.error('❌ Ошибка WASM:', e);
        statusText.textContent = 'Ошибка';
    }
}

// Обработка файла
async function handleFile(file) {
    if (!file || !wasmLoaded) return;
    
    fileName.textContent = file.name;
    fileSize.textContent = `(${(file.size / 1024).toFixed(1)} KB)`;
    fileInfo.classList.add('visible');
    statusText.textContent = 'Обработка...';
    
    try {
        const data = new Uint8Array(await file.arrayBuffer());
        console.log(`📄 ${file.name}, ${data.length} байт`);
        
        // Вызов WASM функций
        if (Module.visualizeFile) {
            console.log(Module.visualizeFile(data));
        }
        
        if (Module.processForWebGL) {
            const info = JSON.parse(Module.processForWebGL(data));
            console.log('📊 Информация:', info);
        }
        
        if (Module.renderToCanvas) {
            Module.renderToCanvas('glCanvas', data);
        }
        
        statusText.textContent = `✅ ${file.name}`;
    } catch (e) {
        console.error('Ошибка:', e);
        statusText.textContent = 'Ошибка';
    }
}

// Очистка
function clearData() {
    fileInfo.classList.remove('visible');
    fileName.textContent = 'Нет файла';
    fileSize.textContent = '';
    statusText.textContent = 'Готов';
    if (Module.clearData) Module.clearData();
}

// События
dropArea.addEventListener('click', () => fileInput.click());
btnSelect.addEventListener('click', (e) => { e.stopPropagation(); fileInput.click(); });
fileInput.addEventListener('change', (e) => { if (e.target.files[0]) handleFile(e.target.files[0]); fileInput.value = ''; });
btnClear.addEventListener('click', clearData);

// Drag & Drop
dropArea.addEventListener('dragover', (e) => { e.preventDefault(); dropArea.classList.add('dragover'); });
dropArea.addEventListener('dragleave', () => dropArea.classList.remove('dragover'));
dropArea.addEventListener('drop', (e) => {
    e.preventDefault();
    dropArea.classList.remove('dragover');
    if (e.dataTransfer.files[0]) handleFile(e.dataTransfer.files[0]);
});

// Запуск
loadWASM();