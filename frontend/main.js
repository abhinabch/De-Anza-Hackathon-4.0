// main.js

document.addEventListener("DOMContentLoaded", () => {
  const tosInput = document.getElementById("tos");
  const outputEl = document.getElementById("output");
  const btnEl = document.getElementById("btn");

  if (!tosInput || !outputEl || !btnEl) {
    console.error("Required DOM elements not found. Check your HTML ids.");
    return;
  }

  btnEl.addEventListener("click", async () => {
    const tosText = tosInput.value.trim();

    if (!tosText) {
      outputEl.textContent = "⚠️ Please enter some text to analyze.";
      outputEl.className = "error";
      return;
    }

    // Show loading state
    btnEl.disabled = true;
    outputEl.textContent = "Analyzing...";
    outputEl.className = "loading";

    try {
      const response = await fetch("http://localhost:8080/analyze", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ tosText }),
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data = await response.json();
      outputEl.textContent = JSON.stringify(data, null, 2);
      outputEl.className = "success";
    } catch (err) {
      outputEl.textContent =
        "❌ Error connecting to backend:\n" +
        err.message +
        "\n\nMake sure the Crow server is running on http://localhost:8080";
      outputEl.className = "error";
    } finally {
      btnEl.disabled = false;
    }
  });
});