function showMessage() {
	// Ask the user for a note
	const note = document.getElementById("myInput").value;
	if (note && note.trim() !== "") {
		const p = document.createElement("p");
		p.textContent = "> " + note;
		document.body.appendChild(p);
	}
}

function sendFile() {
	const fileInput = document.getElementById('fileInput');
	if (!fileInput.files[0]) {
		return;
	}

	const file = fileInput.files[0];

	fetch(`http://localhost/upload?filename=${encodeURIComponent(file.name)}`, {
		method: 'POST',
		body: file
	})
		.then(response => response.text())
		.then(data => console.log(data));
}