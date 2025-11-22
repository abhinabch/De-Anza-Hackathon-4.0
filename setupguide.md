# Terms of Service Analyzer - Setup Guide

## Project Structure

```
tos-analyzer/
├── app.cpp                    # Backend C++ server
├── package.json               # Already exists
├── vite.config.ts            # Already exists
├── tailwind.config.js        # NEW - Create this
├── postcss.config.js         # NEW - Create this
├── index.html                # Already exists (no changes needed)
└── src/
    ├── main.tsx              # NEW - Create this
    ├── App.tsx               # NEW - Create this
    ├── index.css             # NEW - Create this
    └── components/
        └── TOSAnalyzer.tsx   # NEW - Create this
```

## Step 1: Install Dependencies

```bash
# Install all dependencies
npm install

# Install React and TypeScript dependencies
npm install react react-dom
npm install -D tailwindcss postcss autoprefixer typescript @types/react @types/react-dom @types/node

# Initialize Tailwind
npx tailwindcss init -p
```

## Step 2: Create Files

Create all the files from the artifacts above in their respective locations.

## Step 3: Update package.json

Add these to your existing `package.json` if not already present:

```json
{
  "devDependencies": {
    "tailwindcss": "^3.4.1",
    "postcss": "^8.4.33",
    "autoprefixer": "^10.4.16",
    "typescript": "^5.3.3",
    "@types/react": "^18.2.48",
    "@types/react-dom": "^18.2.18"
  }
}
```

## Step 4: Compile and Run Backend

```bash
# Compile the C++ backend
g++ -std=c++14 app.cpp -o server -lboost_system -lpthread

# Run the backend server (port 8080)
./server
```

Keep this terminal running!

## Step 5: Run Frontend

In a **new terminal window**:

```bash
# Start the React development server (port 3000)
npm run dev
```

## Step 6: Access the Application

Open your browser to: **http://localhost:3000**

The React app on port 3000 will communicate with the C++ backend on port 8080.

## Features

✅ Modern, beautiful UI with gradients and animations
✅ File upload support (.txt files)
✅ Real-time character counter
✅ Copy individual clauses to clipboard
✅ Download analysis results as .txt file
✅ Responsive design (works on mobile and desktop)
✅ Loading states and error handling
✅ AI-powered keyword detection

## Testing

1. Paste or upload Terms of Service text
2. Click "Analyze with AI"
3. View highlighted important clauses
4. Copy or download results

## Troubleshooting

**Backend not connecting:**
- Ensure `./server` is running on port 8080
- Check backend terminal for errors
- Test with: `curl http://localhost:8080/test`

**Frontend not loading:**
- Ensure `npm run dev` is running
- Check for port conflicts on 3000
- Verify all npm dependencies installed

**CORS errors:**
- Backend includes CORS headers for cross-origin requests
- If issues persist, check browser console for specific errors