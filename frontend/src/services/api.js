import axios from 'axios'

const API_URL = process.env.REACT_APP_API_URL || '/api/v1'

const client = axios.create({
  baseURL: API_URL,
  timeout: 30000,
})

export const compileCode = async (code, mode = 'both', optimize = false) => {
  try {
    const response = await client.post('/compile', {
      code,
      mode,
      optimize,
    })
    return response.data
  } catch (error) {
    return {
      success: false,
      error: error.message || 'Error compiling code',
      details: error.response?.data?.details || '',
    }
  }
}

export const checkStatus = async () => {
  try {
    const response = await client.get('/status')
    return response.data
  } catch (error) {
    return { success: false, error: error.message }
  }
}

export default client
