/* ======================================
   Microclima V2.1 — Lógica de Calendario
   Factorizado y en Castellano - Minimalista
   ====================================== */

let calEstado = {
    tipo: 'riego',
    volumen: 1000,
    nota: ''
};

// --- Estado de selección del calendario ---
let diaSeleccionado = null;  // Número de día seleccionado (1-31), null = hoy
let eventoAEditar = null;    // Timestamp unix del evento que se está editando

// --- Long press ---
let longPressTimer = null;
const LONG_PRESS_MS = 500;

function initCalendario() {
    console.log('Calendario inicializado');
}

function obtenerFechaSeleccionada() {
    const d = new Date();
    if (diaSeleccionado !== null) {
        d.setDate(diaSeleccionado);
    }
    return d;
}

function formatearFechaInput(date) {
    const anio = date.getFullYear();
    const mes = String(date.getMonth() + 1).padStart(2, '0');
    const diaStr = String(date.getDate()).padStart(2, '0');
    return `${anio}-${mes}-${diaStr}`;
}

function abrirModalCal(dia = null) {
    // Si se pasa un día explícito, usarlo. Si no, usar el día seleccionado o hoy.
    if (dia !== null) {
        diaSeleccionado = dia;
        actualizarChipContextual(dia);
    }

    const fecha = obtenerFechaSeleccionada();
    document.getElementById('calFecha').value = formatearFechaInput(fecha);
    document.getElementById('modalCal').style.display = 'flex';
}

function abrirParaEditar(f, tipo, ml, nota) {
    eventoAEditar = f;
    const d = new Date(f * 1000 + new Date().getTimezoneOffset() * 60000);
    abrirModalCal(d.getDate());
    
    // Seleccionar tipo
    document.querySelectorAll('.event-type').forEach(el => {
        if (el.textContent.toLowerCase().includes(tipo.substring(0,4))) {
            seleccionarTipo(tipo, el);
        }
    });

    // Restaurar valores
    calEstado.volumen = ml;
    document.getElementById('volDisplay').textContent = ml;
    document.getElementById('calNota').value = nota || '';

    // Cambiar título
    document.querySelector('.modal-title').textContent = 'Editar Acción';
}

function cerrarModalCal() {
    document.getElementById('modalCal').style.display = 'none';
    eventoAEditar = null;
    document.querySelector('.modal-title').textContent = 'Registrar Acción';
    document.getElementById('calNota').value = '';
}

function seleccionarTipo(tipo, el) {
    calEstado.tipo = tipo;
    // UI Update
    document.querySelectorAll('.event-type').forEach(item => item.classList.remove('active'));
    el.classList.add('active');
    
    // Cambiar etiqueta de unidad y valores por defecto según el tipo
    const unitEl = document.getElementById('volUnit');
    const displayEl = document.getElementById('volDisplay');

    if (tipo === 'riego') {
        unitEl.textContent = 'ml (total)';
        calEstado.volumen = 1000;
    } else if (tipo === 'fertilizante') {
        unitEl.textContent = 'ml / Litro';
        calEstado.volumen = 2; // Dosis común
    }
    
    displayEl.textContent = calEstado.volumen;

    // Si es una nota, ocultar volumen
    const volControl = document.getElementById('volControl');
    if (tipo === 'nota') {
        volControl.style.opacity = '0.3';
        volControl.style.pointerEvents = 'none';
    } else {
        volControl.style.opacity = '1';
        volControl.style.pointerEvents = 'auto';
    }
}

function cambiarVolumen(delta) {
    // Definir el "paso" (step) según el tipo
    let paso = 100;
    if (calEstado.tipo === 'fertilizante') {
        paso = 1; // De a 1ml o 1g
    }
    
    // Aplicar el delta corregido por el paso
    let direccion = delta > 0 ? 1 : -1;
    calEstado.volumen += (direccion * paso);

    if (calEstado.volumen < 0) calEstado.volumen = 0;
    if (calEstado.volumen > 10000) calEstado.volumen = 10000;
    
    document.getElementById('volDisplay').textContent = calEstado.volumen;
}

// Soporte para presión continua (Sensibilidad ajustada a 250ms)
let volInterval = null;
function startVolChange(e, delta) {
    if (e.cancelable) e.preventDefault(); // Evita selección de texto en Android
    cambiarVolumen(delta);
    if (volInterval) clearInterval(volInterval);
    volInterval = setInterval(() => cambiarVolumen(delta), 250);
}

function stopVolChange() {
    if (volInterval) {
        clearInterval(volInterval);
        volInterval = null;
    }
}

window.addEventListener('mouseup', stopVolChange);
window.addEventListener('touchend', stopVolChange);

async function registrarEnCalendario() {
    const nota = document.getElementById('calNota').value;
    const fechaInput = document.getElementById('calFecha').value;
    let fechaUnix = 0;
    
    if (fechaInput) {
        const [y, m, d] = fechaInput.split('-');
        const actual = new Date();
        const dObj = new Date(y, m - 1, d, actual.getHours(), actual.getMinutes(), actual.getSeconds()); // Usar hora actual para que el unix sea único
        fechaUnix = Math.floor(dObj.getTime() / 1000);
        // Compensar timezone: el ESP32 usa hora local, no UTC
        fechaUnix -= new Date().getTimezoneOffset() * 60;
    }

    const body = {
        tipo: calEstado.tipo,
        ml: calEstado.tipo === 'nota' ? 0 : calEstado.volumen,
        nota: nota,
        fechaUnix: fechaUnix
    };

    const btn = document.querySelector('.btn-registrar');
    const originalText = btn.textContent;
    btn.textContent = 'Guardando...';
    btn.disabled = true;

    try {
        // Si estamos editando, primero eliminamos el original
        if (eventoAEditar !== null) {
            await fetch(`/api/calendario?f=${eventoAEditar}`, { method: 'DELETE' });
        }

        const res = await fetch('/api/calendario', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body)
        });

        if (res.ok) {
            btn.textContent = '✅ Registrado';
            // Refrescar historial inmediatamente
            cargarHistorial();
            setTimeout(() => {
                cerrarModalCal();
                btn.textContent = originalText;
                btn.disabled = false;
                document.getElementById('calNota').value = '';
            }, 1000);
        } else {
            throw new Error('Error en servidor');
        }
    } catch (err) {
        console.error('Error al registrar:', err);
        btn.textContent = '❌ Error';
        btn.style.background = 'var(--accent-red)';
        setTimeout(() => {
            btn.textContent = originalText;
            btn.style.background = 'var(--accent-green)';
            btn.disabled = false;
        }, 2000);
    }
}

async function cargarHistorial() {
    const ahora = new Date();
    const mes = ahora.getMonth() + 1;
    const anio = ahora.getFullYear();
    const cacheKey = `cal_cache_${anio}_${mes}`;

    try {
        const res = await fetch(`/api/historial?mes=${mes}&anio=${anio}`);
        let data = { eventos: [] };
        if (res.ok) {
            data = await res.json();
            // Guardar copia local para modo offline
            localStorage.setItem(cacheKey, JSON.stringify(data));
        } else {
            const cache = localStorage.getItem(cacheKey);
            if (cache) data = JSON.parse(cache);
        }
        dibujarCalendario(data.eventos);
    } catch (err) {
        console.error('Error al cargar historial:', err);
        document.getElementById('calMonthTitle').textContent = "Error: " + err.message;
        const cache = localStorage.getItem(cacheKey);
        if (cache) {
            try { dibujarCalendario(JSON.parse(cache).eventos); } catch(e){}
        } else {
            dibujarCalendario([]);
        }
    }
}

function actualizarChipContextual(dia) {
    const chip = document.getElementById('calChipFecha');
    if (!chip) return;

    if (dia === null) {
        chip.style.display = 'none';
        return;
    }

    const ahora = new Date();
    const fecha = new Date(ahora.getFullYear(), ahora.getMonth(), dia);
    const dias = ['Domingo', 'Lunes', 'Martes', 'Miercoles', 'Jueves', 'Viernes', 'Sabado'];
    const meses = ['Ene', 'Feb', 'Mar', 'Abr', 'May', 'Jun', 'Jul', 'Ago', 'Sep', 'Oct', 'Nov', 'Dic'];

    const esHoy = dia === ahora.getDate();
    const label = esHoy 
        ? `📅 Hoy, ${dias[fecha.getDay()]} ${dia}` 
        : `📅 ${dias[fecha.getDay()]} ${dia} ${meses[fecha.getMonth()]}`;

    chip.textContent = label;
    chip.style.display = 'block';
}

function dibujarCalendario(eventos) {
    const ahora = new Date();
    const anio = ahora.getFullYear();
    const mes = ahora.getMonth();
    
    const meses = ["Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"];
    document.getElementById('calMonthTitle').textContent = `${meses[mes]} ${anio}`;

    const container = document.getElementById('calendarContainer');
    container.innerHTML = '';

    let primerDia = new Date(anio, mes, 1).getDay();
    primerDia = (primerDia === 0) ? 6 : primerDia - 1;

    for (let e = 0; e < primerDia; e++) {
        const empty = document.createElement('div');
        empty.className = 'cal-day empty';
        container.appendChild(empty);
    }

    const diasEnMes = new Date(anio, mes + 1, 0).getDate();

    for (let i = 1; i <= diasEnMes; i++) {
        const div = document.createElement('div');
        div.className = 'cal-day';
        if (i === ahora.getDate()) div.classList.add('today');
        
        const tieneEventos = eventos.some(e => {
            // Compensar el offset local para que el calendario muestre el día real guardado
            const d = new Date(e.f * 1000 + new Date().getTimezoneOffset() * 60000);
            return d.getDate() === i;
        });
        
        if (tieneEventos) div.classList.add('has-events');
        
        div.textContent = i;

        // --- Tap: seleccionar día y mostrar detalles ---
        const diaNum = i;
        div.addEventListener('click', () => {
            document.querySelectorAll('.cal-day').forEach(d => d.classList.remove('selected'));
            div.classList.add('selected');
            diaSeleccionado = diaNum;
            actualizarChipContextual(diaNum);
            mostrarDetallesDia(diaNum, eventos);
        });

        // --- Long press: abrir modal directo con esa fecha ---
        div.addEventListener('touchstart', (e) => {
            longPressTimer = setTimeout(() => {
                // Vibración háptica si el navegador lo soporta
                if (navigator.vibrate) navigator.vibrate(50);
                // Seleccionar visualmente el día
                document.querySelectorAll('.cal-day').forEach(d => d.classList.remove('selected'));
                div.classList.add('selected');
                diaSeleccionado = diaNum;
                actualizarChipContextual(diaNum);
                // Abrir modal directo con la fecha
                abrirModalCal(diaNum);
            }, LONG_PRESS_MS);
        }, { passive: true });

        div.addEventListener('touchend', () => {
            clearTimeout(longPressTimer);
        });

        div.addEventListener('touchmove', () => {
            clearTimeout(longPressTimer);
        });

        // Long press con mouse (para testing en desktop)
        div.addEventListener('mousedown', (e) => {
            longPressTimer = setTimeout(() => {
                document.querySelectorAll('.cal-day').forEach(d => d.classList.remove('selected'));
                div.classList.add('selected');
                diaSeleccionado = diaNum;
                actualizarChipContextual(diaNum);
                abrirModalCal(diaNum);
            }, LONG_PRESS_MS);
        });

        div.addEventListener('mouseup', () => {
            clearTimeout(longPressTimer);
        });

        div.addEventListener('mouseleave', () => {
            clearTimeout(longPressTimer);
        });

        container.appendChild(div);
    }

    // Si hay un día seleccionado previamente, re-seleccionarlo visualmente
    if (diaSeleccionado !== null && diaSeleccionado <= diasEnMes) {
        const dias = container.querySelectorAll('.cal-day:not(.empty)');
        if (dias[diaSeleccionado - 1]) {
            dias[diaSeleccionado - 1].classList.add('selected');
        }
        actualizarChipContextual(diaSeleccionado);
        // Mostrar detalles para que se auto-refresque al guardar/borrar
        mostrarDetallesDia(diaSeleccionado, eventos);
    } else {
        // Mostrar detalles del día actual por defecto
        mostrarDetallesDia(ahora.getDate(), eventos);
    }

    // Hint de primera vez
    mostrarHintLongPress();
}

function mostrarDetallesDia(dia, eventos) {
    const list = document.getElementById('eventDetails');
    list.innerHTML = `<div style="display:flex; justify-content:space-between; align-items:center; margin-bottom: 10px;">
        <h4 style="margin:0;">Eventos del día ${dia}</h4>
        <button onclick="abrirModalCal(${dia})" style="padding:4px 12px; background:var(--accent-green); border:none; border-radius:12px; color:#000; font-weight:bold; font-size:0.75rem; cursor:pointer;">+ Añadir</button>
    </div>`;
    
    const eventosDia = eventos.filter(e => {
        const d = new Date(e.f * 1000 + new Date().getTimezoneOffset() * 60000);
        return d.getDate() === dia;
    });

    if (eventosDia.length === 0) {
        list.innerHTML += '<div class="empty-state">No hay registros este día</div>';
        return;
    }

    const iconos = { riego: '\uD83D\uDCA7', fertilizante: '\uD83E\uDDEA', nota: '\uD83D\uDCDD' };

    eventosDia.forEach(e => {
        const item = document.createElement('div');
        item.className = 'historial-item';
        const dObj = new Date(e.f * 1000 + new Date().getTimezoneOffset() * 60000);
        const hora = dObj.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
        
        item.innerHTML = `
            <div class="hist-icon">${iconos[e.t] || '•'}</div>
            <div class="hist-info">
                <div class="hist-tipo">
                    ${e.t} 
                    <span style="float:right; font-size:0.7rem; color:var(--text-muted)">${hora}</span>
                </div>
                ${e.ml > 0 ? `<div class="hist-val">${e.ml} ${e.t === 'riego' ? 'ml total' : 'ml/L'}</div>` : ''}
                ${e.n ? `<div class="hist-nota">"${e.n}"</div>` : ''}
                <div style="margin-top: 8px; text-align: right;">
                    <button onclick="abrirParaEditar(${e.f}, '${e.t}', ${e.ml}, '${e.n.replace(/'/g, "\\'")}')" style="background:none; border:none; color:var(--accent-blue); font-size:0.8rem; cursor:pointer; text-decoration:underline; margin-right:15px;">Editar</button>
                    <button onclick="borrarEvento(${e.f})" style="background:none; border:none; color:var(--accent-red); font-size:0.8rem; cursor:pointer; text-decoration:underline;">Eliminar</button>
                </div>
            </div>
        `;
        list.appendChild(item);
    });
}

async function borrarEvento(fechaUnix) {
    if(!confirm("¿Seguro que deseas eliminar este registro?")) return;
    
    try {
        const res = await fetch(`/api/calendario?f=${fechaUnix}`, { method: 'DELETE' });
        if (res.ok) {
            cargarHistorial(); // Refrescar la vista
        } else {
            alert("No se pudo eliminar el evento");
        }
    } catch (err) {
        console.error(err);
        alert("Error de conexión");
    }
}

// --- Hint de primera vez ---
function mostrarHintLongPress() {
    if (localStorage.getItem('hint_longpress_shown')) return;

    const chip = document.getElementById('calChipFecha');
    if (!chip) return;

    chip.textContent = '💡 Mantené presionado un día para registrar rápido';
    chip.style.display = 'block';
    chip.classList.add('hint-pulse');

    // Ocultar después de 4 segundos
    setTimeout(() => {
        chip.classList.remove('hint-pulse');
        if (diaSeleccionado === null) {
            chip.style.display = 'none';
        } else {
            actualizarChipContextual(diaSeleccionado);
        }
    }, 4000);

    localStorage.setItem('hint_longpress_shown', '1');
}

window.onclick = function(event) {
    const modal = document.getElementById('modalCal');
    if (event.target == modal) {
        cerrarModalCal();
    }
}
