document.addEventListener('DOMContentLoaded', () => {
  const analyzeBtn = document.getElementById('analyzeBtn');
  const status = document.getElementById('status');
  const results = document.getElementById('results');
  const risksList = document.getElementById('risksList');
  const loading = document.getElementById('loading');
  const errorDiv = document.getElementById('error');

  // This is where your code goes
  analyzeBtn.addEventListener('click', async () => {
    const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
    
    loading.style.display = 'block';
    results.style.display = 'none';
    errorDiv.style.display = 'none';
    analyzeBtn.disabled = true;
    
    const loadingText = document.getElementById('loadingText');
    const progressBar = document.getElementById('progressBar');
    
    try {
      // Step 1: Auto-scroll (20%)
      loadingText.textContent = 'Step 1/3: Auto-scrolling page...';
      progressBar.style.width = '20%';
      status.textContent = 'Auto-scrolling to load complete TOS...';
      status.className = 'status scanning';

      await chrome.scripting.executeScript({
        target: { tabId: tab.id },
        func: autoScrollPage
      });

      await new Promise(resolve => setTimeout(resolve, 2000));

      // Step 2: Extract text (50%)
      loadingText.textContent = 'Step 2/3: Extracting text...';
      progressBar.style.width = '50%';
      status.textContent = 'Extracting Terms of Service text...';

      const [{ result }] = await chrome.scripting.executeScript({
        target: { tabId: tab.id },
        func: extractTOSText
      });

      if (!result || result.length < 100) {
        throw new Error('No Terms of Service found. Try visiting a TOS/Privacy Policy page.');
      }

      // Step 3: Analyze (80%)
      loadingText.textContent = 'Step 3/3: AI Analysis in progress...';
      progressBar.style.width = '80%';
      status.textContent = `Analyzing ${Math.round(result.length / 1000)}KB of text with AI...`;

      const response = await fetch('http://localhost:8080/analyze', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ tosText: result })
      });

      if (!response.ok) {
        throw new Error('Backend server not responding. Start server on port 8080');
      }

      const data = await response.json();
      
      // Complete (100%)
      progressBar.style.width = '100%';
      displayResults(data);

    } catch (error) {
      errorDiv.textContent = `❌ ${error.message}`;
      errorDiv.style.display = 'block';
      status.textContent = 'Analysis failed';
      status.className = 'status warning';
    } finally {
      loading.style.display = 'none';
      analyzeBtn.disabled = false;
      progressBar.style.width = '0%';
    }
  });

  // Auto-scroll function
  function autoScrollPage() {
    return new Promise((resolve) => {
      let totalHeight = 0;
      let distance = 100;
      let scrollDelay = 100;
      
      const timer = setInterval(() => {
        const scrollHeight = document.body.scrollHeight;
        window.scrollBy(0, distance);
        totalHeight += distance;

        if (totalHeight >= scrollHeight) {
          clearInterval(timer);
          window.scrollTo(0, 0);
          resolve();
        }
      }, scrollDelay);
    });
  }

  // Extract TOS text
  function extractTOSText() {
    const tosSelectors = [
      '[class*="terms"]',
      '[class*="privacy"]',
      '[class*="policy"]',
      '[id*="terms"]',
      '[id*="privacy"]',
      'main',
      'article',
      '.content',
      '#content'
    ];
    
    for (const selector of tosSelectors) {
      const element = document.querySelector(selector);
      if (element && element.innerText.length > 500) {
        return element.innerText;
      }
    }
    
    const mainContent = document.querySelector('main, article, [role="main"]');
    if (mainContent) {
      return mainContent.innerText;
    }
    
    return document.body.innerText;
  }

  // Display results
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
      data.highlights.forEach((highlight) => {
        const riskItem = document.createElement('div');
        riskItem.className = 'risk-item';
        
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
    
    if (data.summary) {
      const summaryDiv = document.createElement('div');
      summaryDiv.style.cssText = 'background: #e8f4f8; padding: 12px; border-radius: 8px; margin-bottom: 15px; font-size: 13px; line-height: 1.5;';
      summaryDiv.innerHTML = `<strong>📊 Summary:</strong><br>${data.summary}`;
      results.insertBefore(summaryDiv, results.firstChild);
    }
  }
});
