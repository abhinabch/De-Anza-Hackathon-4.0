// Automatically detect TOS pages and show warning badge
(function() {
  const url = window.location.href.toLowerCase();
  const body = document.body.innerText.toLowerCase();
  
  const tosKeywords = ['terms of service', 'terms and conditions', 
                       'user agreement', 'privacy policy', 'terms of use'];
  
  let isTOSPage = tosKeywords.some(keyword => 
    url.includes(keyword.replace(/\s/g, '-')) || 
    url.includes(keyword.replace(/\s/g, '')) ||
    body.includes(keyword)
  );
  
  if (isTOSPage) {
    // Notify background script
    chrome.runtime.sendMessage({ action: 'tosDetected' });
  }
})();