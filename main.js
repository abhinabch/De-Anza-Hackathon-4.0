const form = document.getElementById("tos-form");
const summaryEl = document.getElementById("summary");
const tosOutputEl = document.getElementById("tos-output");
const resultsSection = document.getElementById("results");
const errorSection = document.getElementById("error");

form.addEventListener("submit", async (e) => {
  e.preventDefault();
  hideError();
  resultsSection.classList.add("hidden");

  const tosText = document.getElementById("tos-input").value.trim();
  if (!tosText) {
    showError("Please paste some TOS text first.");
    return;
  }

  try {
    const res = await fetch("http://localhost:8080/analyze", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ tosText }),
    });

    if (!res.ok) {
      const txt = await res.text();
      showError(`Server error (${res.status}): ${txt}`);
      return;
    }

    const data = await res.json();

    summaryEl.textContent = data.summary || "No summary provided.";
    tosOutputEl.innerHTML = highlightText(tosText, data.highlights || []);
    resultsSection.classList.remove("hidden");
  } catch (err) {
    console.error(err);
    showError("Could not connect to backend on http://localhost:8080.");
  }
});

function highlightText(text, highlights) {
  // replace important clauses with <mark> highlights
  let html = escapeHtml(text);

  for (const raw of highlights) {
    if (!raw) continue;
    const pattern = new RegExp(
      escapeRegExp(raw.trim()),
      "gi"
    );
    html = html.replace(pattern, (match) => `<mark>${match}</mark>`);
  }

  return html;
}

function escapeHtml(str) {
  return str
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");
}

function escapeRegExp(str) {
  return str.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function showError(msg) {
  errorSection.textContent = msg;
  errorSection.classList.remove("hidden");
}

function hideError() {
  errorSection.classList.add("hidden");
  errorSection.textContent = "";
}