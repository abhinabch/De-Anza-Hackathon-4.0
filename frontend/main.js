document.getElementById("btn").addEventListener("click", async () => {
  const tosText = document.getElementById("tos").value;

  try {
    const response = await fetch("http://localhost:8080/analyze", {
      method: "POST",                          // 👈 important
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ tosText })        // 👈 JSON with tosText
    });

    const data = await response.json();
    document.getElementById("output").textContent =
      JSON.stringify(data, null, 2);
  } catch (err) {
    document.getElementById("output").textContent =
      "Error: " + err;
  }
});