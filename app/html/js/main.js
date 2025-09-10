function showMessage() {
	// Ask the user for a note
	const note = document.getElementById("myInput").value;
	if (note && note.trim() !== "") {
		const p = document.createElement("p");
		p.textContent = "> " + note;
		document.body.appendChild(p);
	}
}
