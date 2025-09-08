function showMessage() {
    // Ask the user for a note
    const note = prompt("Please leave your note:");

    if (note && note.trim() !== "") {
        const p = document.createElement("p");
        p.textContent = "> " + note;
        document.body.appendChild(p);
    }
}

