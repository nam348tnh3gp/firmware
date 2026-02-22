export enum Page {
	Home,
	Flasher,
	Donate,
	MyBruce,
	AppStore
}

import { writable, get } from 'svelte/store';
export const current_page = writable(Page.Home);

// Categories store
interface App {
	name: string;
	description: string;
	category: string;
	version: string;
	commit: string;
	owner: string;
	repo: string;
	path: string;
	files: string[] | Array<{source: string, destination: string}>;
	slug: string;
	lastUpdated: number;
	metadataPath: string;
	"supported-devices"?: string | string[];
	"supported-screen-size"?: string;
	"slug-name"?: string;
}

interface Category {
	name: string;
	slug: string;
	count: number;
	apps: App[];
	lastUpdated: number;
}

interface AllCategoriesResponse {
	generated: number;
	generatedISO: string;
	totalCategories: number;
	totalApps: number;
	categories: Category[];
}

export const allCategoriesData = writable<AllCategoriesResponse | null>(null);
export const categories = writable<Category[]>([]);
export const categoryApps = writable<App[]>([]);
export const isLoadingData = writable<boolean>(false);
export const selectedCategory = writable<string>('All');

// Device interfaces and stores
interface Device {
	name: string;
	'screen-size': string;
}

export const supportedDevices = writable<Device[]>([]);
export const selectedDevice = writable<string>('All Devices');
export const filteredApps = writable<App[]>([]);
export const filteredCategories = writable<Category[]>([]);

// Function to calculate device-filtered category counts
function calculateFilteredCategories(deviceName: string, deviceScreenSize: string) {
	const data = get(allCategoriesData);
	const currentCategories = get(categories);
	if (!data || !currentCategories.length) return;
	
	const filteredCategoriesList = currentCategories.map(category => {
		const compatibleApps = category.apps
			.filter(app => isAppCompatibleWithDevice(app, deviceName, deviceScreenSize))
			.sort((a, b) => a.name.localeCompare(b.name));
		
		return {
			...category,
			count: compatibleApps.length,
			apps: compatibleApps
		};
	});
	
	filteredCategories.set(filteredCategoriesList);
}

// Function to check if an app is compatible with a device
function isAppCompatibleWithDevice(app: App, deviceName: string, deviceScreenSize: string): boolean {
	if (deviceName === 'All Devices') return true;
	
	// For themes, also check screen size compatibility
	if (app.category === 'Themes') {
		const appScreenSize = app['supported-screen-size'];
		if (appScreenSize && deviceScreenSize !== 'any') {
			// Normalize screen size format (handle × vs x)
			const normalizeScreenSize = (size: string) => size.replace('×', 'x').toLowerCase();
			if (normalizeScreenSize(appScreenSize) !== normalizeScreenSize(deviceScreenSize)) {
				return false;
			}
		}
	}
	
	// Check device compatibility
	if (!app['supported-devices']) return true; // If not specified, available for all devices
	
	const supportedDevices = app['supported-devices'];
	
	// Handle string (single device or regex)
	if (typeof supportedDevices === 'string') {
		// Try as regex first
		try {
			const regex = new RegExp(supportedDevices, 'i');
			return regex.test(deviceName);
		} catch {
			// If regex fails, treat as exact string match
			return supportedDevices.toLowerCase() === deviceName.toLowerCase();
		}
	}
	
	// Handle array of device names
	if (Array.isArray(supportedDevices)) {
		return supportedDevices.some(device => 
			device.toLowerCase() === deviceName.toLowerCase()
		);
	}
	
	return false;
}

// Function to apply device filter to current category apps
export function applyDeviceFilter(deviceName: string) {
	selectedDevice.set(deviceName);
	
	// Find the selected device object to get screen size
	const devices = get(supportedDevices);
	const selectedDeviceObj = devices.find(device => device.name === deviceName);
	const deviceScreenSize = selectedDeviceObj ? selectedDeviceObj['screen-size'] : 'any';
	
	// Calculate filtered categories with device-specific counts
	calculateFilteredCategories(deviceName, deviceScreenSize);
	
	const currentCategoryApps = get(categoryApps);
	const filtered = currentCategoryApps
		.filter(app => isAppCompatibleWithDevice(app, deviceName, deviceScreenSize))
		.sort((a, b) => a.name.localeCompare(b.name));
	
	filteredApps.set(filtered);
}
export async function loadAllData() {
	try {
		isLoadingData.set(true);
		
		// Load both categories/apps and supported devices
		const [categoriesResponse, devicesResponse] = await Promise.all([
			fetch('https://brucedevices.github.io/App-Store-Data/releases/category-all.json'),
			fetch('https://brucedevices.github.io/App-Store-Data/supported-devices-new.json')
		]);
		
		const data: AllCategoriesResponse = await categoriesResponse.json();
		const devices: Device[] = await devicesResponse.json();
		
		// Create 'All' category with all apps
		const allApps = data.categories.flatMap(cat => cat.apps);
		const allCategory: Category = {
			name: 'All',
			slug: 'all',
			count: data.totalApps,
			apps: allApps,
			lastUpdated: Math.max(...data.categories.map(cat => cat.lastUpdated))
		};
		
		// Add 'All' category at the beginning
		const categoriesWithAll = [allCategory, ...data.categories];
		
		// Create "All Devices" device object and add at the beginning
		const allDevicesOption: Device = { name: 'All Devices', 'screen-size': 'any' };
		const devicesWithAll = [allDevicesOption, ...devices];
		
		allCategoriesData.set(data);
		categories.set(categoriesWithAll);
		supportedDevices.set(devicesWithAll);
		
		// Initialize filtered categories with all devices
		calculateFilteredCategories('All Devices', 'any');
	} catch (error) {
		console.error('Failed to load categories and apps:', error);
		// Fallback to empty data
		allCategoriesData.set(null);
		categories.set([]);
		supportedDevices.set([{ name: 'All Devices', 'screen-size': 'any' }]);
	} finally {
		isLoadingData.set(false);
	}
}

// Function to filter apps for a specific category
export function filterAppsForCategory(categorySlug: string, categoryName: string) {
	const data = get(allCategoriesData);
	if (!data) return;
	
	let apps: App[];
	if (categorySlug === 'all') {
		// Show all apps
		apps = data.categories.flatMap(cat => cat.apps).sort((a, b) => a.name.localeCompare(b.name));
	} else {
		// Show apps for specific category
		const category = data.categories.find(cat => cat.slug === categorySlug);
		apps = category ? category.apps.sort((a, b) => a.name.localeCompare(b.name)) : [];
	}
	
	categoryApps.set(apps);
	selectedCategory.set(categoryName);
	
	// Apply current device filter
	const currentDevice = get(selectedDevice);
	applyDeviceFilter(currentDevice);
}

// Function to clear selected category and apps
export function clearCategorySelection() {
	selectedCategory.set('');
	categoryApps.set([]);
}
