// YModem protocol constants
const CTRL_D = 0x04; // CTRL+D (End of Transmission) - used for cancellation
const EOT = 0x04; // End of Transmission

export type InstallStage =
	| 'connecting'
	| 'getting_info'
	| 'erasing'
	| 'writing'
	| 'verifying'
	| 'rebooting'
	| 'error'
	| 'success';

export interface InstallProgress {
	stage: InstallStage;
	progress: number;
	message: string;
	error?: string;
	speedBps?: number;
}

export interface AppFile {
	source: string;
	sourceFile: string;
	destination: string;
}

export class InstallService {
	private port: SerialPort | null = null;
	private reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
	private writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
	private crcAlreadyReceived: boolean = false;
	private debug: boolean = false;
	private installEnabled: boolean = false; // Flag to disable install functionality
	private packetSize: number = 128; // Configurable YModem packet size

	/**
	 * Check if install functionality is enabled
	 */
	isInstallEnabled(): boolean {
		return this.installEnabled;
	}

	/**
	 * Enable or disable install functionality
	 */
	setInstallEnabled(enabled: boolean): void {
		this.installEnabled = enabled;
	}

	/**
	 * Check if the browser supports Web Serial API
	 */
	isWebSerialSupported(): boolean {
		return 'serial' in navigator;
	}

	/**
	 * Get the reason why Web Serial API is not supported (if applicable)
	 */
	getUnsupportedReason(): string | null {
		if (this.isWebSerialSupported()) {
			return null;
		}

		// Check for common browser types
		const userAgent = navigator.userAgent.toLowerCase();
		
		if (userAgent.includes('firefox')) {
			return 'Firefox does not support Web Serial API. Please use Chrome, Edge, or another Chromium-based browser.';
		} else if (userAgent.includes('safari') && !userAgent.includes('chrome')) {
			return 'Safari does not support Web Serial API. Please use Chrome, Edge, or another Chromium-based browser.';
		} else {
			return 'Your browser does not support Web Serial API. Please use Chrome, Edge, or another Chromium-based browser.';
		}
	}

	async connectDevice(): Promise<boolean> {
		try {
			if (this.debug) console.log('[Install Service] Checking Web Serial API support...');
			if (!('serial' in navigator)) {
				if (this.debug) console.error('[Install Service] Web Serial API not supported in this browser');
				throw new Error('Web Serial API is not supported in this browser');
			}

			if (this.debug) console.log('[Install Service] Requesting serial port access...');
			this.port = await navigator.serial.requestPort();
			if (this.debug) console.log('[Install Service] Serial port selected, opening connection...');
			
			await this.port.open({ baudRate: 115200 });
			if (this.debug) console.log('[Install Service] Serial port opened at 115200 baud');

			if (!this.port.readable || !this.port.writable) {
				if (this.debug) console.error('[Install Service] Serial port streams not available');
				throw new Error('Failed to establish serial connection');
			}

			this.reader = this.port.readable.getReader();
			this.writer = this.port.writable.getWriter();
			if (this.debug) console.log('[Install Service] Serial streams established successfully');

			// Ensure device is at command prompt
			await this.ensureCommandPrompt();

			return true;
		} catch (error) {
			if (this.debug) console.error('[Install Service] Failed to connect to device:', error);
			return false;
		}
	}

	private async ensureCommandPrompt(): Promise<void> {
		if (this.debug) console.log('[Install Service] Ensuring device is at command prompt...');
		
		if (this.debug) console.log('[Install Service] Waiting for device to settle...');
		await new Promise(resolve => setTimeout(resolve, 2000));
		
		// Send a newline to trigger a prompt display
		if (this.debug) console.log('[Install Service] Sending newline to trigger prompt...');
		const encoder = new TextEncoder();
		await this.writer!.write(encoder.encode('\n'));
		await new Promise(resolve => setTimeout(resolve, 1000));

		// Read any response and check for command-ready patterns
		if (this.debug) console.log('[Install Service] Reading device state...');
		const decoder = new TextDecoder();
		let response = '';
		const timeout = 3000;
		const startTime = Date.now();

		while (Date.now() - startTime < timeout) {
			try {
				const { value, done } = await Promise.race([
					this.reader!.read(),
					new Promise<{ value: undefined, done: true }>((_, reject) => 
						setTimeout(() => reject(new Error('Read timeout')), 200)
					)
				]);

				if (done) {
					break;
				}
				
				if (value) {
					const chunk = decoder.decode(value, { stream: true });
					response += chunk;
					if (this.debug) console.log(`[Install Service] Received: "${chunk.replace(/\r\n/g, '\\r\\n').replace(/\r/g, '\\r').replace(/\n/g, '\\n')}"`);
					
					// Check for various command prompt patterns
					if (response.includes('# ') || response.includes('#\r\n') || response.includes('#\n') || 
						response.includes('COMMAND:') || response.endsWith('#') || response.endsWith('> ')) {
						if (this.debug) console.log('[Install Service] Command interface detected, device ready');
						return;
					}
				}
			} catch (error) {
				// Continue waiting
				break;
			}
		}

		if (this.debug) console.log('[Install Service] Device settled, ready to send commands');
	}

	async disconnectDevice(): Promise<void> {
		try {
			if (this.debug) console.log('[Install Service] Disconnecting from device...');
			
			if (this.reader) {
				if (this.debug) console.log('[Install Service] Closing reader stream...');
				await this.reader.cancel();
				this.reader = null;
			}

			if (this.writer) {
				if (this.debug) console.log('[Install Service] Closing writer stream...');
				await this.writer.close();
				this.writer = null;
			}

			if (this.port) {
				if (this.debug) console.log('[Install Service] Closing serial port...');
				await this.port.close();
				this.port = null;
			}
			
			if (this.debug) console.log('[Install Service] Device disconnected successfully');
		} catch (error) {
			if (this.debug) console.error('[Install Service] Error disconnecting from device:', error);
		}
	}

	private async sendCommand(command: string): Promise<string> {
		if (!this.writer || !this.reader) {
			if (this.debug) console.error('[Install Service] Serial connection not established for command:', command);
			throw new Error('Serial connection not established');
		}

		// Wait a bit to ensure device is ready for commands
		if (this.debug) console.log('[Install Service] Waiting for device to be ready for command...');
		await new Promise(resolve => setTimeout(resolve, 1000));

		if (this.debug) console.log(`[Install Service] Sending command: "${command}"`);
		
		// Send command (use \n terminator)
		const encoder = new TextEncoder();
		const commandBytes = encoder.encode(command + '\n');
		if (this.debug) console.log(`[Install Service] Command bytes: [${Array.from(commandBytes).join(', ')}]`);
		await this.writer.write(commandBytes);
		if (this.debug) console.log(`[Install Service] Sent ${commandBytes.length} bytes to device`);

		// Small delay to ensure command is sent
		await new Promise(resolve => setTimeout(resolve, 500));

		// Read response with timeout - look for YModem ready message
		const decoder = new TextDecoder();
		let response = '';
		const timeout = 10000; // 10 seconds
		const startTime = Date.now();

		if (this.debug) console.log('[Install Service] Waiting for YModem ready response...');
		while (Date.now() - startTime < timeout) {
			try {
				const { value, done } = await Promise.race([
					this.reader.read(),
					new Promise<{ value: undefined, done: true }>((_, reject) => 
						setTimeout(() => reject(new Error('Read timeout')), 500)
					)
				]);

				if (done) {
					if (this.debug) console.log('[Install Service] Device stream ended');
					break;
				}
				if (value) {
					const chunk = decoder.decode(value, { stream: true });
					response += chunk;
					if (this.debug) console.log(`[Install Service] Received chunk: "${chunk.replace(/\r\n/g, '\\r\\n').replace(/\r/g, '\\r').replace(/\n/g, '\\n')}"`);
					
					// Look for YModem ready message
					if (response.includes('Send file using Y-modem protocol')) {
						if (this.debug) console.log('[Install Service] YModem transfer ready detected');
						
						// Check if CRC is mixed in response
						const textLines = response.split('\n');
						const lastLine = textLines[textLines.length - 1];
						if (lastLine.includes('C') || response.endsWith('C') || response.includes('\nC') || response.includes('C[DEBUG]')) {
							if (this.debug) console.log('[Install Service] CRC request found in command response (mixed with debug)');
							this.crcAlreadyReceived = true;
						}
						break;
					}
					
					// Check for command errors
					if (response.includes('Error:') || response.includes('ERROR:')) {
						if (this.debug) console.error(`[Install Service] Command error detected: ${response}`);
						
						// If it's the "Command not found" error, it means the device didn't receive our command properly
						if (response.includes('Command not found')) {
							throw new Error(`Command not received properly by device. Check connection and try again.`);
						}
						
						throw new Error(`Command failed: ${response}`);
					}
					
					// If we've been waiting for a while and have some response, check what we got
					if (Date.now() - startTime > 5000 && response.length > 0) {
						if (response.includes('COMMAND:') && !response.includes('Send file using Y-modem protocol')) {
							if (this.debug) console.error(`[Install Service] Command did not trigger YModem: ${response}`);
							throw new Error(`YModem command not recognized by device: ${response}`);
						}
					}
				}
			} catch {
				// Continue reading
			}
		}

		if (this.debug) console.log(`[Install Service] Final response: "${response.trim().replace(/\r\n/g, '\\r\\n')}"`);
		
		// Check if we got a meaningful response
		if (!response || response.trim().length === 0) {
			if (this.debug) console.error('[Install Service] No response received from device');
			throw new Error('No response from device - check connection and retry the installation');
		}
		
		return response.trim();
	}

	private calculateCRC16(data: Uint8Array): number {
		let crc = 0x0000;
		for (let i = 0; i < data.length; i++) {
			crc ^= data[i] << 8;
			for (let j = 0; j < 8; j++) {
				if (crc & 0x8000) {
					crc = (crc << 1) ^ 0x1021;
				} else {
					crc <<= 1;
				}
			}
			crc &= 0xFFFF; // Keep 16-bit
		}
		return crc;
	}

	private async waitForByte(expectedByte: number, timeoutMs: number = 5000): Promise<boolean> {
		if (this.debug) console.log(`[Install Service] Waiting for byte: 0x${expectedByte.toString(16).padStart(2, '0')}`);
		const startTime = Date.now();
		
		// Collect all data - CRC might be mixed with text
		let allData = new Uint8Array(0);
		
		while (Date.now() - startTime < timeoutMs) {
			try {
				const { value, done } = await Promise.race([
					this.reader!.read(),
					new Promise<{ value: undefined, done: true }>((_, reject) => 
						setTimeout(() => reject(new Error('Read timeout')), 200)
					)
				]);

				if (done) break;
				if (value && value.length > 0) {
					// Append to our data collection
					const newData = new Uint8Array(allData.length + value.length);
					newData.set(allData);
					newData.set(value, allData.length);
					allData = newData;

					// Check every byte in the collected data for our expected byte
					for (let i = 0; i < allData.length; i++) {
						const byte = allData[i];
						if (byte === expectedByte) {
							if (this.debug) console.log(`[Install Service] Found expected byte 0x${expectedByte.toString(16).padStart(2, '0')} at position ${i} in data stream`);
							
							// Log some context around the found byte for debugging
							const start = Math.max(0, i - 10);
							const end = Math.min(allData.length, i + 10);
							const context = Array.from(allData.slice(start, end)).map(b => 
								b >= 32 && b <= 126 ? String.fromCharCode(b) : `\\x${b.toString(16).padStart(2, '0')}`
							).join('');
							if (this.debug) console.log(`[Install Service] Context around CRC byte: "${context}"`);
							
							return true;
						}
					}

					// Also try to decode as text for debugging
					try {
						const decoder = new TextDecoder('utf-8', { fatal: false });
						const text = decoder.decode(value);
						if (text.length > 0) {
							if (this.debug) console.log(`[Install Service] Received text chunk: "${text.replace(/\r\n/g, '\\r\\n').replace(/\r/g, '\\r').replace(/\n/g, '\\n')}"`);
						}
					} catch {
						// Not valid text, continue
					}
				}
			} catch {
				// Continue waiting on timeout
			}
			
			// Small delay to avoid overwhelming the loop
			await new Promise(resolve => setTimeout(resolve, 50));
		}
		
		// Final check - log all collected data for debugging
		if (allData.length > 0) {
			if (this.debug) console.log(`[Install Service] Total bytes collected: ${allData.length}`);
			if (this.debug) console.log(`[Install Service] All bytes: [${Array.from(allData).map(b => b.toString(16).padStart(2, '0')).join(', ')}]`);
			
			// Try to show as text too
			try {
				const decoder = new TextDecoder('utf-8', { fatal: false });
				const fullText = decoder.decode(allData);
				if (this.debug) console.log(`[Install Service] All data as text: "${fullText.replace(/\r\n/g, '\\r\\n').replace(/\r/g, '\\r').replace(/\n/g, '\\n')}"`);
			} catch {
				if (this.debug) console.log('[Install Service] Could not decode all data as text');
			}
		}
		
		console.warn(`[Install Service] Timeout waiting for byte 0x${expectedByte.toString(16)} after ${timeoutMs}ms`);
		return false;
	}

	private async waitForSingleByteResponse(timeoutMs: number = 2500): Promise<number | null> {
		if (this.debug) console.log('[Install Service] Waiting for single byte response (ACK/NAK/CTRL+D)...');
		const startTime = Date.now();
		
		// Collect all data - ACK/NAK might be mixed with debug text
		let allData = new Uint8Array(0);
		
		while (Date.now() - startTime < timeoutMs) {
			try {
				const { value, done } = await Promise.race([
					this.reader!.read(),
					new Promise<{ value: undefined, done: true }>((_, reject) => 
						setTimeout(() => reject(new Error('Read timeout')), 200) // Shorter timeout for individual reads
					)
				]);

				if (done) break;
				if (value && value.length > 0) {
					// Append to our data collection
					const newData = new Uint8Array(allData.length + value.length);
					newData.set(allData);
					newData.set(value, allData.length);
					allData = newData;

					// Check every byte in the collected data for protocol responses
					for (let i = 0; i < allData.length; i++) {
						const byte = allData[i];
						
						// Only return actual protocol bytes
						if (byte === 0x06) { // ACK
							if (this.debug) console.log(`[Install Service] Found ACK (0x06) at position ${i} in data stream`);
							return 0x06;
						} else if (byte === 0x15) { // NAK  
							if (this.debug) console.log(`[Install Service] Found NAK (0x15) at position ${i} in data stream`);
							return 0x15;
						} else if (byte === 0x04) { // CTRL+D
							if (this.debug) console.log(`[Install Service] Found CTRL+D (0x04) at position ${i} in data stream`);
							return 0x04;
						}
					}

					// Also try to decode as text for debugging
					try {
						const decoder = new TextDecoder('utf-8', { fatal: false });
						const text = decoder.decode(value);
						if (text.length > 0) {
							if (this.debug) console.log(`[Install Service] Received text chunk: "${text.replace(/\r\n/g, '\\r\\n').replace(/\r/g, '\\r').replace(/\n/g, '\\n')}"`);
						}
					} catch {
						// Not valid text, continue
					}
				}
			} catch {
				// Continue waiting on timeout - this is normal when no data is available
			}
			
			// Small delay to avoid overwhelming the loop
			await new Promise(resolve => setTimeout(resolve, 50));
		}
		
		// Final check - log all collected data for debugging
		if (allData.length > 0) {
			if (this.debug) console.log(`[Install Service] Total bytes collected while waiting for response: ${allData.length}`);
			if (this.debug) console.log(`[Install Service] Response bytes: [${Array.from(allData).map(b => `0x${b.toString(16).padStart(2, '0')}`).join(', ')}]`);
			
			// Try to show as text too
			try {
				const decoder = new TextDecoder('utf-8', { fatal: false });
				const fullText = decoder.decode(allData);
				if (this.debug) console.log(`[Install Service] Response as text: "${fullText.replace(/\r\n/g, '\\r\\n').replace(/\r/g, '\\r').replace(/\n/g, '\\n')}"`);
			} catch {
				if (this.debug) console.log('[Install Service] Could not decode response as text');
			}
		}
		
		console.warn(`[Install Service] Timeout waiting for response after ${timeoutMs}ms`);
		return null;
	}
	private async sendFileViaYModemWithRetry(
		filePath: string,
		fileBytes: Uint8Array,
		progressCallback?: (progress: number, speedBps: number) => void
	): Promise<boolean> {
		const maxFileRetries = 2; // Try the file transfer a total of 3 times (1 initial + 2 retries)
		let fileRetries = 0;

		while (fileRetries <= maxFileRetries) {
			try {
				if (fileRetries > 0) {
					console.warn(`[Install Service] File-level retry ${fileRetries}/${maxFileRetries} for "${filePath}"`);
					
					// Send CTRL+D sequence to ensure any previous session is cancelled
					try {
						if (this.debug) console.log('[Install Service] Sending CTRL+D sequence to cancel any previous transfer...');
						await this.writer!.write(new Uint8Array([CTRL_D, CTRL_D, CTRL_D, CTRL_D, CTRL_D]));
						await new Promise(resolve => setTimeout(resolve, 1000));
					} catch (cancelError) {
						console.warn('[Install Service] Failed to send pre-retry CTRL+D sequence:', cancelError);
					}
				}
				
				// Attempt to send the file
				await this.sendFileViaYModem(filePath, fileBytes, progressCallback);
				return true; // File sent successfully
			} catch (error) {
				console.warn(`[Install Service] Error during YModem transfer for "${filePath}" (attempt ${fileRetries + 1}/${maxFileRetries + 1}):`, error);
				fileRetries++;

				if (fileRetries <= maxFileRetries) {
					// Will retry on next iteration - cancellation already handled above
					if (this.debug) console.log(`[Install Service] Will retry file transfer for "${filePath}" (${fileRetries}/${maxFileRetries})...`);
					
					// Additional delay to let device settle after error
					await new Promise(resolve => setTimeout(resolve, 500));
				} else {
					console.error(`[Install Service] File transfer for "${filePath}" failed after ${maxFileRetries + 1} attempts (${maxFileRetries} retries).`);
					throw error; // Re-throw the error to fail the installation
				}
			}
		}
		// This should never be reached since we always return or throw in the loop
		throw new Error(`Unexpected end of retry loop for file: ${filePath}`);
	}

	private async sendFileViaYModem(
		filePath: string, 
		fileBytes: Uint8Array, 
		progressCallback?: (progress: number, speedBps: number) => void
	): Promise<boolean> {
		try {
			if (this.debug) console.log(`[Install Service] Starting YModem transfer for: "${filePath}"`);
			if (this.debug) console.log(`[Install Service] File size: ${fileBytes.length} bytes`);
			
			// Calculate total blocks needed for progress tracking
			const blockSize = this.packetSize;
			const totalBlocks = Math.ceil(fileBytes.length / blockSize);
			let blocksCompleted = 0;
			const transferStartTime = Date.now();
			let bytesSent = 0;
			
			const updateProgress = (isFinal: boolean = false) => {
				if (progressCallback) {
					const progress = totalBlocks > 0 ? blocksCompleted / totalBlocks : 0;
					const elapsedTime = (Date.now() - transferStartTime) / 1000; // in seconds
					const speedBps = elapsedTime > 0 ? bytesSent / elapsedTime : 0;
					
					if (this.debug) console.log(`[Install Service] updateProgress calling callback with: ${progress.toFixed(3)}, speed: ${speedBps.toFixed(0)} B/s`);
					progressCallback(progress, isFinal ? 0 : speedBps);
				}
			};
			
			// Reset CRC flag
			this.crcAlreadyReceived = false;
			
			// Start YModem transfer
			const command = `storage ymodem "${filePath}"`;
			const response = await this.sendCommand(command);
			
			if (response.includes('error') || response.includes('Error')) {
				if (this.debug) console.error(`[Install Service] YModem command failed with response: "${response}"`);
				throw new Error(`YModem command failed: ${response}`);
			}

			if (this.debug) console.log(`[Install Service] YModem command successful`);
			
			// Check if CRC was already received in command response
			if (this.crcAlreadyReceived) {
				if (this.debug) console.log('[Install Service] Using CRC request from command response');
			} else {
				// Wait for separate CRC request
				if (this.debug) console.log('[Install Service] Waiting for CRC request (0x43)...');
				const crcReceived = await this.waitForByte(0x43, 5000);
				if (!crcReceived) {
					throw new Error('Timeout waiting for CRC request from device');
				}
			}
			
			if (this.debug) console.log('[Install Service] CRC request confirmed, starting YModem transfer');

			// Send Block 0 (header block) with filename and file size
			if (this.debug) console.log('[Install Service] Sending Block 0 (header) with filename and file size');
			
			// Create Block 0 data: filename + null + file size + null + padding
			const block0Data = new Uint8Array(this.packetSize);
			let offset = 0;
			
			// Extract just the filename from the full path for Block 0
			const filename = filePath.split('/').pop() || 'file.js';
			const encoder = new TextEncoder(); // Only for Block 0 filename/size encoding
			const filenameBytes = encoder.encode(filename);
			
			// Copy filename
			block0Data.set(filenameBytes, offset);
			offset += filenameBytes.length;
			
			// Add null terminator after filename
			block0Data[offset++] = 0x00;
			
			// Add file size as ASCII string
			const fileSize = fileBytes.length;
			const fileSizeStr = fileSize.toString();
			const fileSizeBytes = encoder.encode(fileSizeStr);
			block0Data.set(fileSizeBytes, offset);
			offset += fileSizeBytes.length;
			
			// Add null terminator after file size
			block0Data[offset++] = 0x00;
			
			// Fill remaining bytes with zeros (not 0x1A for header block)
			// block0Data is already zero-filled by default
			
			if (this.debug) console.log(`[Install Service] Block 0 contains: filename="${filename}", size="${fileSizeStr}"`);
			
			// Send Block 0
			await this.sendBlock(0, block0Data);

			// Use file bytes directly (no re-encoding)
			if (this.debug) console.log(`[Install Service] Using ${fileBytes.length} bytes of file data directly`);
			if (this.debug) console.log(`[Install Service] Will send ${totalBlocks} data blocks of ${blockSize} bytes each`);
			
			// Send data blocks (using 128-byte SOH blocks)
			let blockNum = 1;
			offset = 0; // Reset offset for file data processing
			
			while (offset < fileBytes.length) {
				const remaining = fileBytes.length - offset;
				const currentBlockSize = Math.min(blockSize, remaining);
				const blockData = new Uint8Array(blockSize);
				
				// Copy data to block directly from file bytes
				blockData.set(fileBytes.subarray(offset, offset + currentBlockSize));
				
				// Pad remaining bytes with 0x1A (EOF) if needed
				if (currentBlockSize < blockSize) {
					blockData.fill(0x1A, currentBlockSize);
				}
				
				if (this.debug) console.log(`[Install Service] Sending block ${blockNum} (${currentBlockSize} bytes data) - ${blocksCompleted + 1}/${totalBlocks}`);
				
				// Debug: Show first few bytes of this block for troubleshooting
				const firstBytes = Array.from(blockData.slice(0, Math.min(16, blockData.length)));
				if (this.debug) console.log(`[Install Service] Block ${blockNum} first 16 bytes: [${firstBytes.map(b => `0x${b.toString(16).padStart(2, '0')}`).join(', ')}]`);
				
				// Send block using the common sendBlock method
				await this.sendBlock(blockNum, blockData);
				
				// Update progress after successful block
				blocksCompleted++;
				bytesSent = offset + currentBlockSize;
				updateProgress();
				
				offset += currentBlockSize;
				blockNum = (blockNum + 1) % 256; // Wrap 0-255, but 0 after 255 is just a normal data block
			}

			// Send EOT (End of Transmission)
			if (this.debug) console.log('[Install Service] Sending EOT (End of Transmission)');
			await this.writer!.write(new Uint8Array([0x04])); // EOT
			
			// Wait for final ACK with retries
			let finalAckReceived = false;
			let eotRetries = 0;
			
			while (!finalAckReceived && eotRetries < 3) {
				const response = await this.waitForSingleByteResponse(5000);
				if (response === 0x06) { // ACK
					finalAckReceived = true;
				} else if (response === 0x15) { // NAK
					eotRetries++;
					if (this.debug) console.log(`[Install Service] EOT NAK'd, retrying (${eotRetries}/3)`);
					await this.writer!.write(new Uint8Array([0x04])); // Resend EOT
				} else {
					eotRetries++;
					if (this.debug) console.log(`[Install Service] No response to EOT, retrying (${eotRetries}/3)`);
					await this.writer!.write(new Uint8Array([0x04])); // Resend EOT
				}
			}

			if (!finalAckReceived) {
				throw new Error('No final ACK received after EOT');
			}

			// Final progress update to show 100%
			updateProgress(true);

			if (this.debug) console.log(`[Install Service] YModem transfer completed for: "${filePath}"`);
			return true;
		} catch (error) {
			if (this.debug) console.error(`[Install Service] YModem transfer failed for "${filePath}":`, error);
			throw error; // Re-throw to trigger file-level retry
		}
	}

	private async sendBlock(blockNum: number, data: Uint8Array): Promise<void> {
		const maxRetries = 3;
		let retries = 0;
		
		while (retries < maxRetries) {
			// Build the block
			const block = new Uint8Array(3 + this.packetSize + 2); // SOH + block# + ~block# + data + CRC
			let pos = 0;
			
			// SOH header for configurable packet size blocks
			block[pos++] = 0x01; // SOH
			
			// Block number and complement
			block[pos++] = blockNum & 0xFF;
			block[pos++] = (~blockNum) & 0xFF;
			
			// Data payload (configurable packet size)
			block.set(data, pos);
			pos += this.packetSize;
			
			// Calculate CRC-16 over data only
			const crc = this.calculateCRC16(data);
			block[pos++] = (crc >> 8) & 0xFF; // CRC high byte
			block[pos++] = crc & 0xFF;        // CRC low byte
			
			if (this.debug) console.log(`[Install Service] Sending block ${blockNum}: SOH + ${blockNum} + ${(~blockNum) & 0xFF} + ${this.packetSize} bytes + CRC(0x${crc.toString(16).padStart(4, '0')})`);
			
			// Send the block
			await this.writer!.write(block);
			
			// Wait for response
			const response = await this.waitForSingleByteResponse(5000);
			if (response === 0x06) { // ACK
				if (this.debug) console.log(`[Install Service] Block ${blockNum} acknowledged`);
				return; // Success - exit function
			} else if (response === 0x15) { // NAK
				retries++;
				console.warn(`[Install Service] Block ${blockNum} NAK'd, retry ${retries}/${maxRetries}`);
				if (retries >= maxRetries) {
					throw new Error(`Failed to send block ${blockNum} after ${maxRetries} retries`);
				}
				// Add small delay before retry to let device recover
				await new Promise(resolve => setTimeout(resolve, 100));
			} else {
				retries++;
				console.warn(`[Install Service] No valid response for block ${blockNum}, retry ${retries}/${maxRetries}`);
				if (retries >= maxRetries) {
					throw new Error(`Failed to send block ${blockNum} after ${maxRetries} retries`);
				}
				// Add small delay before retry to let device recover
				await new Promise(resolve => setTimeout(resolve, 100));
			}
		}
		
		// If we reach here, all retries have been exhausted
		throw new Error(`Failed to send block ${blockNum} after ${maxRetries} retries - no valid response received`);
	}

	private async sendDataBlock(blockNum: number, data: Uint8Array): Promise<void> {
		const block = new Uint8Array(3 + this.packetSize + 2); // SOH + block# + ~block# + data + CRC
		let pos = 0;
		
		// SOH header for configurable packet size blocks
		block[pos++] = 0x01; // SOH
		
		// Block number and complement
		block[pos++] = blockNum & 0xFF;
		block[pos++] = (~blockNum) & 0xFF;
		
		// Data payload (configurable packet size)
		block.set(data, pos);
		pos += this.packetSize;
		
		// Calculate CRC-16 over data only
		const crc = this.calculateCRC16(data);
		block[pos++] = (crc >> 8) & 0xFF; // CRC high byte
		block[pos++] = crc & 0xFF;        // CRC low byte
		
		if (this.debug) console.log(`[Install Service] Sending block ${blockNum}: SOH + ${blockNum} + ${(~blockNum) & 0xFF} + ${this.packetSize} bytes + CRC(0x${crc.toString(16).padStart(4, '0')})`);
		
		await this.writer!.write(block);
	}

	async installApp(
		appName: string,
		files: AppFile[],
		progressCallback: (progress: InstallProgress) => void
	): Promise<boolean> {
		if (this.debug) console.log(`[Install Service] Starting installation of "${appName}" with ${files.length} file(s)`);
		
		try {
			// Check if install is enabled first
			if (!this.installEnabled) {
				if (this.debug) console.error('[Install Service] Install functionality is currently disabled');
				progressCallback({
					stage: 'error',
					progress: 0,
					message: 'Install functionality not implemented',
					error: 'Direct installation is being developed. Please use the download option instead.'
				});
				return false;
			}

			// Check Web Serial API support first
			if (!this.isWebSerialSupported()) {
				const reason = this.getUnsupportedReason();
				if (this.debug) console.error('[Install Service] Web Serial API not supported:', reason);
				progressCallback({
					stage: 'error',
					progress: 0,
					message: 'Browser not supported for direct installation',
					error: reason || 'Web Serial API not supported'
				});
				return false;
			}

			if (!this.port) {
				// Try to connect if not already connected
				if (this.debug) console.log('[Install Service] No existing connection, attempting to connect...');
				progressCallback({
					stage: 'connecting',
					progress: 0,
					message: 'Connecting to device...'
				});

				const connected = await this.connectDevice();
				if (!connected) {
					if (this.debug) console.error('[Install Service] Failed to connect to device');
					progressCallback({
						stage: 'error',
						progress: 0,
						message: 'Failed to connect to device',
						error: 'Device connection failed'
					});
					return false;
				}
			}

			if (this.debug) console.log('[Install Service] Connection established, starting file processing...');
			progressCallback({
				stage: 'uploading',
				progress: 0,
				message: `Starting upload of ${files.length} file(s)...`
			});

			// Track global progress to prevent backwards movement
			let globalProgress = 0;

			for (let i = 0; i < files.length; i++) {
				const file = files[i];
				// Split 100% evenly among all files
				const fileStartProgress = (i / files.length) * 100;
				const fileEndProgress = ((i + 1) / files.length) * 100;
				const fileProgressRange = fileEndProgress - fileStartProgress;

				if (this.debug) console.log(`[Install Service] Processing file ${i + 1}/${files.length}: "${file.destination}"`);
				if (this.debug) console.log(`[Install Service] Source URL: "${file.source}"`);
				if (this.debug) console.log(`[Install Service] File progress range: ${fileStartProgress.toFixed(1)}% - ${fileEndProgress.toFixed(1)}%`);

				progressCallback({
					stage: 'uploading',
					progress: fileStartProgress,
					message: `Downloading ${file.sourceFile}...`
				});

				// Download file content as binary to preserve exact bytes
				if (this.debug) console.log(`[Install Service] Fetching file from: "${file.source}"`);
				const response = await fetch(file.source);
				if (!response.ok) {
					if (this.debug) console.error(`[Install Service] Failed to fetch file: ${response.status} ${response.statusText}`);
					throw new Error(`Failed to download ${file.destination}`);
				}

				const fileBytes = new Uint8Array(await response.arrayBuffer());
				if (this.debug) console.log(`[Install Service] Downloaded ${fileBytes.length} bytes from "${file.source}"`);

				// Upload file to device using YModem with progress callback
				if (this.debug) console.log(`[Install Service] Starting upload to device: "${file.destination}"`);
				if (this.debug) console.log(`[Install Service] Progress calculation debug: fileStartProgress=${fileStartProgress}, fileEndProgress=${fileEndProgress}, fileProgressRange=${fileProgressRange}`);
				
				const success = await this.sendFileViaYModemWithRetry(
					file.destination, 
					fileBytes, 
					(blockProgress, speedBps) => {
						// Map block progress (0-1) to this file's allocated progress range
						const calculatedProgress = fileStartProgress + (fileProgressRange * blockProgress);
						
						// Ensure global progress only moves forward
						const currentProgress = Math.max(globalProgress, calculatedProgress);
						globalProgress = currentProgress;
						
						if (this.debug) console.log(`[Install Service] Custom callback received blockProgress=${blockProgress.toFixed(3)}`);
						if (this.debug) console.log(`[Install Service] Calculated: fileStart=${fileStartProgress}, range=${fileProgressRange}, calculated=${calculatedProgress.toFixed(1)}, final=${currentProgress.toFixed(1)}`);
						if (this.debug) console.log(`[Install Service] Sending to UI: progress=${currentProgress.toFixed(1)}%, message="${Math.round(blockProgress * 100)}%"`);
						
						progressCallback({
							stage: 'uploading',
							progress: currentProgress,
							message: `Uploading ${file.destination} (${Math.round(blockProgress * 100)}%)`,
							speedBps: speedBps
						});
					}
				);

				// File uploaded successfully - continue to next file
				// Ensure progress reaches exactly the end percentage for this file
				progressCallback({
					stage: 'uploading',
					progress: fileEndProgress,
					message: `Completed ${file.destination}`
				});
				
				if (this.debug) console.log(`[Install Service] Successfully uploaded: "${file.destination}"`);
				
				// Small delay to ensure smooth transition to next file (if any)
				if (i < files.length - 1) {
					await new Promise(resolve => setTimeout(resolve, 100));
				}
			}

			if (this.debug) console.log(`[Install Service] Installation completed successfully for "${appName}"`);
			progressCallback({
				stage: 'success',
				progress: 100,
				message: `Successfully installed ${appName}!`,
				speedBps: 0
			});

			return true;
		} catch (error) {
			if (this.debug) console.error(`[Install Service] Installation failed for "${appName}":`, error);
			progressCallback({
				stage: 'error',
				progress: 0,
				message: 'Installation failed',
				error: error instanceof Error ? error.message : 'Unknown error',
				speedBps: 0
			});
			return false;
		} finally {
			// Keep connection open for potential future operations
			// User can manually disconnect if needed
			if (this.debug) console.log('[Install Service] Installation process finished, keeping connection open');
		}
	}
}

// Export singleton instance
export const installService = new InstallService();
