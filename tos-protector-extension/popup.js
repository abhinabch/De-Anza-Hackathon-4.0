document.addEventListener('DOMContentLoaded', () => {
  const analyzeBtn = document.getElementById('analyzeBtn');
  const status = document.getElementById('status');
  const results = document.getElementById('results');
  const risksList = document.getElementById('risksList');
  const loading = document.getElementById('loading');
  const errorDiv = document.getElementById('error');

  analyzeBtn.addEventListener('click', async () => {
    // Get current tab
    const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
    
    // Show loading
    loading.style.display = 'block';
    results.style.display = 'none';
    errorDiv.style.display = 'none';
    analyzeBtn.disabled = true;
    status.textContent = 'Scanning page for Terms of Service...';
    status.className = 'status scanning';

    try {
      // Extract text from page
      const [{ result }] = await chrome.scripting.executeScript({
        target: { tabId: tab.id },
        func: extractTOSText
      });

      if (!result || result.length < 100) {
        throw new Error('No Terms of Service found on this page');
      }

      // Send to backend for analysis
      const response = await fetch('http://localhost:8080/analyze', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ tosText: result })
      });

      if (!response.ok) {
        throw new Error('Backend server not responding. Make sure it\'s running on port 8080');
      }

      const data = await response.json();

      // Display results
      displayResults(data);

    } catch (error) {
      errorDiv.textContent = `Error: ${error.message}`;
      errorDiv.style.display = 'block';
      status.textContent = 'Analysis failed';
      status.className = 'status warning';
    } finally {
      loading.style.display = 'none';
      analyzeBtn.disabled = false;
    }
  });

  function extractTOSText() {
    // Try to find TOS content on the page
    const body = document.body.innerText;
    
    // Look for common TOS indicators
    const tosKeywords = ['terms of service', 'terms and conditions', 'user agreement', 
                         'privacy policy', 'terms of use'];
    
    const lowerBody = body.toLowerCase();
    for (const keyword of tosKeywords) {
      if (lowerBody.includes(keyword)) {
        return body;
      }
    }
    
    return body.substring(0, 10000); // Return first 10k chars
  }

  function displayResults(data) {
    loading.style.display = 'none';
    results.style.display = 'block';

    const risksCount = data.highlights ? data.highlights.length : 0;

    if (risksCount === 0) {
      status.textContent = '✅ No major risks detected';
      status.className = 'status safe';
      results.style.display = 'none';
      return;
    }

    status.textContent = `⚠️ Found ${risksCount} potential risks`;
    status.className = 'status warning';

    risksList.innerHTML = '';
    
    if (data.highlights) {
      data.highlights.forEach(highlight => {
        const riskItem = document.createElement('div');
        riskItem.className = 'risk-item';
        
        // Extract category if present
        const categoryMatch = highlight.match(/\[(.*?)\]/);
        if (categoryMatch) {
          const category = document.createElement('div');
          category.className = 'risk-category';
          category.textContent = categoryMatch[1];
          riskItem.appendChild(category);
          
          const text = document.createElement('div');
          text.textContent = highlight.replace(/\[.*?\]\s*/, '');
          riskItem.appendChild(text);
        } else {
          riskItem.textContent = highlight;
        }
        
        risksList.appendChild(riskItem);
      });
    }
  }
});