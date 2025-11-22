import { useState } from 'react';
import { FileText, Sparkles, AlertCircle, CheckCircle2, Upload, Copy, Download } from 'lucide-react';

export default function TOSAnalyzer() {
  const [tosText, setTosText] = useState('');
  const [loading, setLoading] = useState(false);
  const [result, setResult] = useState<{ summary: string; highlights: string[] } | null>(null);
  const [error, setError] = useState('');

  const analyzeTerms = async () => {
    if (!tosText.trim()) {
      setError('Please enter Terms of Service text to analyze');
      return;
    }

    setLoading(true);
    setError('');
    setResult(null);

    try {
      const response = await fetch('http://localhost:8080/analyze', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ tosText })
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data = await response.json();
      setResult(data);
    } catch (err) {
      setError('Failed to connect to backend. Make sure the server is running on http://localhost:8080');
      console.error(err);
    } finally {
      setLoading(false);
    }
  };

  const handleFileUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file) {
      const reader = new FileReader();
      reader.onload = (event) => {
        setTosText(event.target?.result as string);
      };
      reader.readAsText(file);
    }
  };

  const copyToClipboard = (text: string) => {
    navigator.clipboard.writeText(text);
  };

  const downloadResults = () => {
    if (!result) return;
    const content = `Terms of Service Analysis\n\n${result.summary}\n\nKey Clauses:\n${result.highlights.join('\n\n')}`;
    const blob = new Blob([content], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'tos-analysis.txt';
    a.click();
    URL.revokeObjectURL(url);
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 via-indigo-50 to-purple-50">
      {/* Header */}
      <header className="bg-white shadow-sm border-b border-gray-200">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center space-x-3">
              <div className="bg-gradient-to-br from-indigo-500 to-purple-600 p-2 rounded-lg">
                <FileText className="w-6 h-6 text-white" />
              </div>
              <div>
                <h1 className="text-2xl font-bold text-gray-900">Terms of Service Analyzer</h1>
                <p className="text-sm text-gray-500">AI-Powered Legal Document Analysis</p>
              </div>
            </div>
            <div className="flex items-center space-x-2 bg-indigo-50 px-3 py-2 rounded-lg">
              <Sparkles className="w-4 h-4 text-indigo-600" />
              <span className="text-sm font-medium text-indigo-600">AI Powered</span>
            </div>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {/* Left Column - Input */}
          <div className="space-y-6">
            <div className="bg-white rounded-xl shadow-lg border border-gray-200 p-6">
              <div className="flex items-center justify-between mb-4">
                <h2 className="text-xl font-semibold text-gray-900">Input Terms of Service</h2>
                <label className="cursor-pointer">
                  <input
                    type="file"
                    accept=".txt"
                    onChange={handleFileUpload}
                    className="hidden"
                  />
                  <div className="flex items-center space-x-2 bg-indigo-50 hover:bg-indigo-100 transition-colors px-3 py-2 rounded-lg">
                    <Upload className="w-4 h-4 text-indigo-600" />
                    <span className="text-sm font-medium text-indigo-600">Upload File</span>
                  </div>
                </label>
              </div>

              <textarea
                value={tosText}
                onChange={(e) => setTosText(e.target.value)}
                placeholder="Paste your Terms of Service text here or upload a .txt file...

Example: We collect data and cookies. Your personal information may be shared with third parties. We are not liable for any damages. You agree to binding arbitration for dispute resolution."
                className="w-full h-96 p-4 border border-gray-300 rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-transparent resize-none font-mono text-sm"
              />

              <div className="mt-4 flex items-center justify-between">
                <span className="text-sm text-gray-500">
                  {tosText.length} characters
                </span>
                <button
                  onClick={analyzeTerms}
                  disabled={loading || !tosText.trim()}
                  className="flex items-center space-x-2 bg-gradient-to-r from-indigo-600 to-purple-600 hover:from-indigo-700 hover:to-purple-700 disabled:from-gray-400 disabled:to-gray-400 text-white px-6 py-3 rounded-lg font-medium transition-all shadow-md hover:shadow-lg disabled:cursor-not-allowed"
                >
                  <Sparkles className="w-5 h-5" />
                  <span>{loading ? 'Analyzing...' : 'Analyze with AI'}</span>
                </button>
              </div>
            </div>

            {/* Info Card */}
            <div className="bg-blue-50 border border-blue-200 rounded-xl p-4">
              <div className="flex items-start space-x-3">
                <AlertCircle className="w-5 h-5 text-blue-600 mt-0.5 flex-shrink-0" />
                <div className="text-sm text-blue-800">
                  <p className="font-medium mb-1">How it works:</p>
                  <ul className="list-disc list-inside space-y-1 text-blue-700">
                    <li>AI analyzes your Terms of Service document</li>
                    <li>Identifies important clauses about privacy, liability, fees, etc.</li>
                    <li>Highlights key legal terms you should be aware of</li>
                    <li>Provides a quick summary of critical information</li>
                  </ul>
                </div>
              </div>
            </div>
          </div>

          {/* Right Column - Results */}
          <div className="space-y-6">
            <div className="bg-white rounded-xl shadow-lg border border-gray-200 p-6">
              <div className="flex items-center justify-between mb-4">
                <h2 className="text-xl font-semibold text-gray-900">Analysis Results</h2>
                {result && (
                  <div className="flex items-center space-x-2">
                    <button
                      onClick={downloadResults}
                      className="flex items-center space-x-1 bg-gray-100 hover:bg-gray-200 transition-colors px-3 py-2 rounded-lg"
                    >
                      <Download className="w-4 h-4 text-gray-600" />
                      <span className="text-sm font-medium text-gray-600">Download</span>
                    </button>
                  </div>
                )}
              </div>

              {/* Default State */}
              {!result && !error && !loading && (
                <div className="h-96 flex flex-col items-center justify-center text-center text-gray-400">
                  <FileText className="w-16 h-16 mb-4 opacity-50" />
                  <p className="text-lg font-medium">No analysis yet</p>
                  <p className="text-sm mt-2">Enter Terms of Service text and click "Analyze with AI"</p>
                </div>
              )}

              {/* Loading State */}
              {loading && (
                <div className="h-96 flex flex-col items-center justify-center">
                  <div className="animate-spin rounded-full h-12 w-12 border-4 border-indigo-200 border-t-indigo-600 mb-4"></div>
                  <p className="text-gray-600 font-medium">Analyzing your Terms of Service...</p>
                  <p className="text-sm text-gray-500 mt-2">This may take a few seconds</p>
                </div>
              )}

              {/* Error State */}
              {error && (
                <div className="bg-red-50 border border-red-200 rounded-lg p-4">
                  <div className="flex items-start space-x-3">
                    <AlertCircle className="w-5 h-5 text-red-600 mt-0.5 flex-shrink-0" />
                    <div>
                      <p className="font-medium text-red-800">Error</p>
                      <p className="text-sm text-red-700 mt-1">{error}</p>
                    </div>
                  </div>
                </div>
              )}

              {/* Results */}
              {result && (
                <div className="space-y-4 max-h-96 overflow-y-auto">
                  {/* Summary */}
                  <div className="bg-gradient-to-br from-green-50 to-emerald-50 border border-green-200 rounded-lg p-4">
                    <div className="flex items-start space-x-3">
                      <CheckCircle2 className="w-5 h-5 text-green-600 mt-0.5 flex-shrink-0" />
                      <div>
                        <p className="font-medium text-green-900">Summary</p>
                        <p className="text-sm text-green-800 mt-1">{result.summary}</p>
                      </div>
                    </div>
                  </div>

                  {/* Highlights */}
                  <div>
                    <h3 className="font-semibold text-gray-900 mb-3 flex items-center">
                      <Sparkles className="w-4 h-4 mr-2 text-indigo-600" />
                      Key Clauses Found ({result.highlights.length})
                    </h3>
                    <div className="space-y-3">
                      {result.highlights.map((highlight, index) => (
                        <div
                          key={index}
                          className="bg-gradient-to-br from-indigo-50 to-purple-50 border border-indigo-200 rounded-lg p-4 hover:shadow-md transition-shadow"
                        >
                          <div className="flex items-start justify-between">
                            <div className="flex-1">
                              <div className="flex items-center space-x-2 mb-2">
                                <span className="bg-indigo-600 text-white text-xs font-bold px-2 py-1 rounded">
                                  {index + 1}
                                </span>
                                <span className="text-xs font-medium text-indigo-600">Important Clause</span>
                              </div>
                              <p className="text-sm text-gray-800">{highlight}</p>
                            </div>
                            <button
                              onClick={() => copyToClipboard(highlight)}
                              className="ml-3 p-2 hover:bg-white rounded-lg transition-colors flex-shrink-0"
                            >
                              <Copy className="w-4 h-4 text-gray-500" />
                            </button>
                          </div>
                        </div>
                      ))}
                    </div>
                  </div>

                  {result.highlights.length === 0 && (
                    <div className="text-center text-gray-500 py-8">
                      <p>No important clauses detected in this text.</p>
                      <p className="text-sm mt-1">Try adding more detailed Terms of Service content.</p>
                    </div>
                  )}
                </div>
              )}
            </div>
          </div>
        </div>

        {/* Footer Info */}
        <div className="mt-8 bg-white rounded-xl shadow-sm border border-gray-200 p-6">
          <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
            <div className="flex items-start space-x-3">
              <div className="bg-indigo-100 p-2 rounded-lg">
                <FileText className="w-5 h-5 text-indigo-600" />
              </div>
              <div>
                <h4 className="font-semibold text-gray-900">Smart Analysis</h4>
                <p className="text-sm text-gray-600 mt-1">AI identifies important legal terms automatically</p>
              </div>
            </div>
            <div className="flex items-start space-x-3">
              <div className="bg-purple-100 p-2 rounded-lg">
                <CheckCircle2 className="w-5 h-5 text-purple-600" />
              </div>
              <div>
                <h4 className="font-semibold text-gray-900">Key Highlights</h4>
                <p className="text-sm text-gray-600 mt-1">Focus on privacy, liability, and user rights</p>
              </div>
            </div>
            <div className="flex items-start space-x-3">
              <div className="bg-pink-100 p-2 rounded-lg">
                <Sparkles className="w-5 h-5 text-pink-600" />
              </div>
              <div>
                <h4 className="font-semibold text-gray-900">Fast & Accurate</h4>
                <p className="text-sm text-gray-600 mt-1">Get results in seconds with AI-powered technology</p>
              </div>
            </div>
          </div>
        </div>
      </main>
    </div>
  );
}