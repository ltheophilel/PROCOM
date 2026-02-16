// --- Initialisation des variables ---
const canvas = document.getElementById('camCanvas');
const ctx = canvas.getContext('2d');
const canvasR = document.getElementById('camCanvasR');
const ctxR = canvasR.getContext('2d');
const out = document.getElementById("output");
const cmdInput = document.getElementById('cmd');
go = false;

// Gestion de la touche Entrée sur l'input
cmdInput.addEventListener("keypress", (e) => {
    if (e.key === "Enter") send();
});

// --- Fonctions ---
async function send_command(command) {
    if(!command) return;
    try {
        await fetch('/send', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({command})
        });
        cmdInput.value = '';
    } catch (e) { console.error("Erreur envoi:", e); }
}

async function send() {
    const command = cmdInput.value;
    if (/^V\d+$/.test(command)) update_values("V", command);
    else if (/^P\d+$/.test(command)) update_values("P_T", command);
    else if (/^T\d+$/.test(command)) update_values("P_T", command);
    else if (/^S\d+$/.test(command)) update_values("S", command);
    else if (/^D\d+$/.test(command)) update_values("D", command);
    else if (/^LED\s+(ON|OFF)$/.test(command)) update_values("LED", command); 
    else if (/^GO$/.test(command)) {
        go = true;
        document.getElementById("GO_STOP").textContent = "STOP";
        document.getElementById("GO_STOP").style.background = "#f44336";
    }
    else if (/^STOP$/.test(command)) {
        go = false;
        document.getElementById("GO_STOP").textContent = "GO";
        document.getElementById("GO_STOP").style.background = "#4CAF50";
    }; 
    send_command(command)
}

async function GO_STOP() {
    try {
        if (!go) {
            send_command("GO");
            go = true;
            document.getElementById("GO_STOP").textContent = "STOP";
            document.getElementById("GO_STOP").style.background = "#f44336";
        } else {
            send_command("STOP");
            go = false;
            document.getElementById("GO_STOP").textContent = "GO";
            document.getElementById("GO_STOP").style.background = "#4CAF50";
        }
    } catch (e) { console.error("Erreur GO_STOP:", e); }
}

async function update_values(which, value) {
    try {
        document.getElementById(which).textContent = value;
    } catch (e) { console.error("Erreur update_values:", e); }
    
}

async function readLoop() {
    try {
        const res = await fetch("/read");
        if (!res.ok) throw new Error("Server error");
        const json = await res.json();

        if (json && json.data && Array.isArray(json.data) && json.data.length > 0) {
            json.data.forEach(line => {
                if (line.startsWith("IMG_DATA:")) {
                    const base64Data = line.split("IMG_DATA:")[1];
                    updateCanvasFromBase64(base64Data, false);
                } else if (line.startsWith("IMG_DATA_R:")) {
                    const base64Data = line.split("IMG_DATA_R:")[1];
                    updateCanvasFromBase64(base64Data, true);
                } else {
                    // On garde les 50 dernières lignes seulement pour la performance
                    if (line.trim() !== "\x00\x00\x00\x00failed" && line.trim() !== "") { // Ignore les lignes vides
                        const newText = document.createElement("div");
                        newText.textContent = `> ${line}`;
                        out.prepend(newText); 
                        if(out.children.length > 50) out.lastChild.remove();
                    }
                }
            });
        }
        // Dans la fonction readLoop()
        // Mise à jour des moteurs (si présents)
        if (json.v_mot1 !== null && json.v_mot1 !== undefined) {
            document.getElementById("v_mot1").textContent = json.v_mot1;
        }
        if (json.v_mot0 !== null && json.v_mot0 !== undefined) {
            document.getElementById("v_mot0").textContent = json.v_mot0;
        }

    } catch (e) { 
        console.error("Erreur lecture:", e); 
    } finally {
        // On relance la boucle même en cas d'erreur, après un petit délai
        setTimeout(readLoop, 50);
    }
}

function updateCanvasFromBase64(base64, R = false) {
    const img = new Image();
    img.onload = function() {
        // Optionnel : lissage d'image (false pour garder l'aspect pixelisé du 80x60)
        const canvas_ = R ? canvasR : canvas;
        const ctx_ = R ? ctxR : ctx;
        ctx_.imageSmoothingEnabled = false;
        ctx_.drawImage(img, 0, 0, canvas_.width, canvas_.height);
    };
    img.src = "data:image/png;base64," + base64;
}

// Lancement de la boucle
readLoop();