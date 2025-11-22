// Update badge when TOS is detected
chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  if (request.action === 'tosDetected') {
    chrome.action.setBadgeText({ 
      text: '!',
      tabId: sender.tab.id 
    });
    chrome.action.setBadgeBackgroundColor({ 
      color: '#FF9800',
      tabId: sender.tab.id 
    });
  }
});

// Clear badge when tab changes
chrome.tabs.onActivated.addListener((activeInfo) => {
  chrome.action.setBadgeText({ text: '' });
});