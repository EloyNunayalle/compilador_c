import React from 'react'

export default function OutputPanel({ output, error, isCompiling }) {
  return (
    <div style={{
      width: '100%',
      height: '100%',
      display: 'flex',
      flexDirection: 'column',
      background: '#1e1e1e',
    }}>
      <div style={{
        display: 'flex',
        alignItems: 'center',
        padding: '8px 16px',
        borderBottom: '1px solid #3e3e42',
        background: '#252526',
        fontSize: '13px',
        fontWeight: '500',
        color: '#cccccc',
        gap: '12px',
      }}>
        <span>Terminal de salida</span>
        {isCompiling && (
          <div style={{
            width: '8px',
            height: '8px',
            borderRadius: '50%',
            background: '#4a9eff',
            animation: 'pulse 1.5s infinite',
          }} />
        )}
      </div>

      <div style={{
        flex: 1,
        overflow: 'auto',
        padding: '12px 16px',
        fontFamily: 'Fira Code',
        fontSize: '12px',
        lineHeight: '1.6',
        background: '#1e1e1e',
      }}>
        {error && (
          <div style={{
            color: '#f48771',
            whiteSpace: 'pre-wrap',
            wordWrap: 'break-word',
          }}>
            <strong>Error:</strong>
            <br />
            {error}
          </div>
        )}

        {output && !error && (
          <pre style={{
            color: '#d4d4d4',
            whiteSpace: 'pre-wrap',
            wordWrap: 'break-word',
            margin: 0,
            fontFamily: 'Fira Code',
            fontSize: '12px',
            lineHeight: '1.6',
          }}>
            {output.split('\n').map((line, idx) => (
              <div key={idx} style={{ display: 'flex' }}>
                <span style={{
                  color: '#858585',
                  marginRight: '16px',
                  minWidth: '40px',
                  textAlign: 'right',
                  userSelect: 'none',
                }}>
                  {idx + 1}
                </span>
                <span style={{
                  flex: 1,
                  color: line.startsWith(';') ? '#6a9955' :
                         line.startsWith('.') ? '#569cd6' : '#d4d4d4',
                }}>
                  {line}
                </span>
              </div>
            ))}
          </pre>
        )}

        {!output && !error && !isCompiling && (
          <div style={{ color: '#666' }}>
            Compila codigo para ver la salida
          </div>
        )}
      </div>

      <style>{`
        @keyframes pulse {
          0%, 100% { opacity: 0.4; }
          50% { opacity: 1; }
        }
      `}</style>
    </div>
  )
}
