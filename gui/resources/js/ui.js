// окна
const loadingWindow = document.getElementsByClassName('window-loading')[0];
const resultsWindow = document.getElementsByClassName('window-results')[0];
const startWindow = document.getElementsByClassName('window-start')[0];
// кнопки управления
const buttonCollapse = document.getElementById('btn-collapse');
const buttonClose = document.getElementById('btn-close');
// системные инпуты
const startDirInput = document.getElementById('input-startDir');
const searchStrInput = document.getElementById('input-searchStr');
// кнопки
const startSearchButton = document.getElementById('btn-search');

class UIController{
	
	constructor(){
		this.currentActiveWindow = null;
	}

	showLoading(){
		this.switchWindow(loadingWindow);
	}
	
	switchWindow(window){
		if(this.currentActiveWindow){this.currentActiveWindow.classList.remove('window_active')}
		window.classList.add('window_active');
		this.currentActiveWindow = window;
	}

	renderResult(result_source){
		const result = [...result_source];
		let html = '';
		for(let i = 0; i < result.length; i++){
			html += `
				<div class="result-line__item result-line__item-result" type="${result[i].type}">
					<p class="result-line__item__type">${result[i].type === 'dir' ? '📁' : '📄'}</p>
					<p class="result-line__item__value">${result[i].path}</p>
				</div>
			`
		}
		window.workspaceController.displayResult(html, result.length);
	}

	showFoundResults(result){
		//result - splitted by <2>
		if(result.length <= 1){
			const html = `
				<div class="result-line__item result-line__item-error">
					<p class="result-line__item__type">❌</p>
					<p class="result-line__item__value">Ничего не найдено</p>
				</div>
			`;
			window.workspaceController.displayResult(html, 0);
			return;
		}
		const result_sys = [];
		for(let i = 0; i < result.length; i++){
			if(result[i] == ''){continue;}
			const currentRes = {};
			const isDirCode = result[i].slice(-3);
			const path = result[i].replace(/<1>/, '');
			currentRes['type'] = isDirCode === '<1>' ? 'dir' : 'file';
			currentRes['path'] = path;
			result_sys.push(currentRes);
		}
		window.app.saveResult(result_sys);
		this.renderResult(result_sys);
	}

	prepareSearch(){
		let startDir = startDirInput.value;
		
		if(!startDir){alert('Введите стартовую директорию'); return}
		startDir = startDir.replace(/\//g, '\\\\');
		const [root, path, ...rest] = startDir.split(':/');
		if(!root){alert('Путь должен быть абсолютным'); return}
		if(rest.length !== 0){alert('Некорректный путь');return}
		if(!startDir.endsWith('\\\\')){startDir = `${startDir}\\\\`;}

		const searchStr = searchStrInput.value;
		if(!searchStr){alert('Строка поиска не может быть пустой'); return}

		window.app.startSearch(startDir, searchStr);
	}
	
	initEvents(){
		buttonCollapse.addEventListener('click', window.app.collapse);
		buttonClose.addEventListener('click', window.app.close);
		startSearchButton.addEventListener('click', this.prepareSearch);
	}

	applyStartDir(dir){
		startDirInput.value = dir;
	}

	init(){
		this.switchWindow(startWindow);
		this.initEvents()
	}
}

window.uiController = new UIController();