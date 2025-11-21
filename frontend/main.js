document.getElementById("btn").addEventListener("click", async () => {
  const tosText = document.getElementById("tos").value;

  try {
    const response = await fetch("/analyze", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ tosText })
    });

    const result = await response.json();
    document.getElementById("output").textContent =
      JSON.stringify(result, null, 2);

  } catch (err) {
    document.getElementById("output").textContent =
      "Error: " + err;
  }
});