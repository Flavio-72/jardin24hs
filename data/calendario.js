/* ======================================
   Microclima V2.1 — Lógica de Calendario
   Factorizado y en Castellano
   ====================================== */

let calEstado = {
    tipo: 'riego',
    volumen: 1000,
    nota: '',
    pID: 0
};

function initCalendario() {
    console.log('Calendario inicializado');
}

function abrirModalCal() {
    document.getElementById('modalCal').style.display = 'flex';
    // Actualizar nombres de las plantas en el selector
    if (window.nombresGeneticas) {
        document.getElementById('pOpt1').textContent = window.nombresGeneticas[0] || 'P1';
        document.getElementById('pOpt2').textContent = window.nombresGeneticas[1] || 'P2';
        document.getElementById('pOpt3').textContent = window.nombresGeneticas[2] || 'P3';
    }
}

function cerrarModalCal() {
    document.getElementById('modalCal').style.display = 'none';
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
    } else if (tipo === 'micorrizas') {
        unitEl.textContent = 'gramos / dosis';
        calEstado.volumen = 1;
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

function seleccionarPlanta(id) {
    calEstado.pID = id;
    document.querySelectorAll('.plant-opt').forEach(opt => opt.classList.remove('active'));
    document.getElementById(`pOpt${id}`).classList.add('active');
}

function cambiarVolumen(delta) {
    // Definir el "paso" (step) según el tipo
    let paso = 100;
    if (calEstado.tipo === 'fertilizante' || calEstado.tipo === 'micorrizas') {
        paso = 1; // De a 1ml o 1g
    }
    
    // Aplicar el delta corregido por el paso
    // Nota: el delta que viene de los botones es +/- 100, lo normalizamos
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

// Seguridad global: si se suelta el toque en cualquier parte, detener incremento
window.addEventListener('mouseup', stopVolChange);
window.addEventListener('touchend', stopVolChange);

async function registrarEnCalendario() {
    const nota = document.getElementById('calNota').value;
    const body = {
        tipo: calEstado.tipo,
        ml: calEstado.tipo === 'nota' ? 0 : calEstado.volumen,
        nota: nota,
        pID: calEstado.pID
    };

    const btn = document.querySelector('.btn-registrar');
    const originalText = btn.textContent;
    btn.textContent = 'Guardando...';
    btn.disabled = true;

    try {
        const res = await fetch('/api/calendario', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body)
        });

        if (res.ok) {
            btn.textContent = '✅ Registrado';
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
    
    const container = document.getElementById('calendarContainer');
    
    // Asegurar que tenemos los nombres de las genéticas
    if (!window.nombresGeneticas) {
        try {
            const resCfg = await fetch('/api/config');
            const cfg = await resCfg.json();
            window.nombresGeneticas = cfg.gens;
        } catch(e) { console.error("Error cargando nombres", e); }
    }
    
    try {
        const res = await fetch(`/cal_${anio}_${String(mes).padStart(2, '0')}.json`);
        let data = { eventos: [] };
        if (res.ok) {
            data = await res.json();
        }
        dibujarCalendario(data.eventos);
    } catch (err) {
        console.error('Error al cargar historial:', err);
        dibujarCalendario([]);
    }
}

function dibujarCalendario(eventos) {
    const ahora = new Date();
    const anio = ahora.getFullYear();
    const mes = ahora.getMonth();
    
    // Título del mes
    const meses = ["Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"];
    document.getElementById('calMonthTitle').textContent = `${meses[mes]} ${anio}`;

    const container = document.getElementById('calendarContainer');
    container.innerHTML = '';

    // Calcular primer día del mes (0=Dom, 1=Lun...)
    let primerDia = new Date(anio, mes, 1).getDay();
    // Ajustar para que Lunes sea el primer día (Lun=0, Dom=6)
    primerDia = (primerDia === 0) ? 6 : primerDia - 1;

    // Rellenar días vacíos al inicio
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
            const d = new Date(e.fecha * 1000);
            return d.getDate() === i;
        });
        
        if (tieneEventos) div.classList.add('has-events');
        
        div.textContent = i;
        div.onclick = () => {
            document.querySelectorAll('.cal-day').forEach(d => d.classList.remove('selected'));
            div.classList.add('selected');
            mostrarDetallesDia(i, eventos);
        };
        container.appendChild(div);
    }
}

function mostrarDetallesDia(dia, eventos) {
    const list = document.getElementById('eventDetails');
    list.innerHTML = `<h4>Eventos del día ${dia}</h4>`;
    
    const eventosDia = eventos.filter(e => {
        const d = new Date(e.fecha * 1000);
        return d.getDate() === dia;
    });

    if (eventosDia.length === 0) {
        list.innerHTML += '<div class="empty-state">No hay registros este día</div>';
        return;
    }

    const iconos = { riego: '💧', fertilizante: '🧪', micorrizas: '🦠', nota: '📝' };

    eventosDia.forEach(e => {
        const item = document.createElement('div');
        item.className = 'historial-item';
        const hora = new Date(e.fecha * 1000).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
        
        let nombrePlanta = 'General';
        if (e.pID > 0 && window.nombresGeneticas) {
            nombrePlanta = window.nombresGeneticas[e.pID - 1] || `Planta ${e.pID}`;
        }

        item.innerHTML = `
            <div class="hist-icon">${iconos[e.tipo] || '•'}</div>
            <div class="hist-info">
                <div class="hist-tipo">
                    ${e.tipo} 
                    <span style="font-size:0.7rem; color:var(--accent-blue); margin-left:8px">[${nombrePlanta}]</span>
                    <span style="float:right; font-size:0.7rem; color:var(--text-muted)">${hora}</span>
                </div>
                ${e.ml > 0 ? `<div class="hist-val">${e.ml} ${e.tipo === 'riego' ? 'ml total' : (e.tipo === 'fertilizante' ? 'ml/L' : 'g/dosis')}</div>` : ''}
                ${e.nota ? `<div class="hist-nota">"${e.nota}"</div>` : ''}
            </div>
        `;
        list.appendChild(item);
    });
}

// Cerrar modal al tocar fuera
window.onclick = function(event) {
    const modal = document.getElementById('modalCal');
    if (event.target == modal) {
        cerrarModalCal();
    }
}
