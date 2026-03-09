const resultsNumberEl = document.getElementById('results-num');
const resultLine = document.getElementsByClassName('window-results__content__result__result-line')[0];
// кнопки
const openFiltersButton = document.getElementById('btn-openFilters');
const resultsBackButton = document.getElementById('btn-resultsBack');
const resultsSaveButton = document.getElementById('btn-resultsSave');
const resultsCopyButton = document.getElementById('btn-resultsCopy');
const addExcludeButton = document.getElementById('btn-exclude');
const addIncludeButton = document.getElementById('btn-include');
const typeInputs = {
	all: document.getElementById('input-type_all'),
	file: document.getElementById('input-type_file'),
	dir: document.getElementById('input-type_dir')
}
const filtersSubmitButton = document.getElementById('btn-filtersSubmit');
const filtersResetButton = document.getElementById('btn-filtersReset');
//вывод
const excludeBody = document.getElementById('filters-block__body-exclude');
const includeBody = document.getElementById('filters-block__body-include');
// инпуты
const excludeInput = document.getElementById('input-exclude');
const includeInput = document.getElementById('input-include');

const workspaceWindow = document.getElementsByClassName('window-results__content')[0];

class WorkSpaceController{

	constructor(){
		this.currentWindowMode = 'result';
		this.currentFilterFileType = 'all';
	}

	switchWindowMode(){
		if(this.currentWindowMode == 'result'){
			this.currentWindowMode = 'filters';
			workspaceWindow.classList.remove('window-results__content_active-result');
			workspaceWindow.classList.add('window-results__content_active-filters');
		}
		else{
			this.currentWindowMode = 'result';
			workspaceWindow.classList.remove('window-results__content_active-filters');
			workspaceWindow.classList.add('window-results__content_active-result');
		}
	}

	displayResult(html, num){
		let currentResultItems = document.getElementsByClassName('result-line__item-result');
		for(let i = 0; i < currentResultItems.length; i++){
			currentResultItems[i].removeEventListener('click', this.openRequestedResult);
		}

		if(this.currentWindowMode == 'filters'){this.switchWindowMode()}
		resultLine.innerHTML = html;
		resultsNumberEl.innerText = num;
		window.uiController.switchWindow(resultsWindow);

		currentResultItems = document.getElementsByClassName('result-line__item-result');
		for(let i = 0; i < currentResultItems.length; i++){
			currentResultItems[i].addEventListener('click', this.openRequestedResult);
		}
	}

	async openRequestedResult(){
		
		const el = this;

		const path_raw = el.querySelector('.result-line__item__value').innerText;
		const path = path_raw.replace(/\//g, '\\');
		const type = el.getAttribute('type');
		
		if(type === 'dir'){
			await Neutralino.os.execCommand(`explorer.exe "${path}"`);
		}
		else{
			await Neutralino.os.execCommand(`explorer.exe /select, "${path}"`);
		}

	}

	lightOpenFiltersButton(){
		openFiltersButton.style.background = '#ffffff';
		openFiltersButton.style.color = '#202020';
		openFiltersButton.style.borderWidth = '0px';
	}
	offOpenFiltersButton(){
		openFiltersButton.style.background = '#202020';
		openFiltersButton.style.color = '#ffffff';
		openFiltersButton.style.borderWidth = '1px';
	}

	handleOpenFiltersClick = () => {
		this.switchWindowMode();
		if(this.currentWindowMode == 'filters'){
			this.lightOpenFiltersButton();
		}
		else{
			this.offOpenFiltersButton();
		}
	}

	formatResult_str(){
		window.app.applyFilters();
		const source = [...window.app.result_filtered];
		let str = '';
		for(let i = 0; i < source.length; i++){
			str+=`${source[i].path}\n`;
		}
		return str;
	}

	handleClickBack = () =>{
		resultLine.innerHTML = '';
		window.uiController.switchWindow(startWindow);
	}
	handleClickSave = async () => {
		const selected = await Neutralino.os.showFolderDialog('Выберите папку для сохранения');
		const writeStr = this.formatResult_str();
		await Neutralino.filesystem.writeFile(`${selected}/search_result.txt`, writeStr);
		console.log('selected: ', selected);
	}
	handleClickCopy = async () => {
		const copyStr = this.formatResult_str();
		await Neutralino.clipboard.writeText(copyStr);
	}

	handleInputFilterType = (type) => {
		if(type === this.currentFilterFileType){return;}
		const currentActiveInput = typeInputs[this.currentFilterFileType];
		const requestedActiveInput = typeInputs[type];
		if(!currentActiveInput || !requestedActiveInput){window.app.crash(`Не удалось найти input filter type, запрошено: ${type}`)}
		currentActiveInput.classList.remove('input-type__item_active');
		requestedActiveInput.classList.add('input-type__item_active');
		window.app.filters.onlyType = type;
		this.currentFilterFileType = type;
	}

	handleRemoveFilterRuleClick = (e) => {
		const target = e.target;
		const type = target.getAttribute('type');
		const value = target.innerText;
		window.app.removeFilterValue({type, value});
		this.displayUpdatedFilters();
	}

	displayUpdatedFilters(){
		const oldInteractive = document.getElementsByClassName('filters-block__data__removable');
		if(oldInteractive.length !== 0){
			for(let i = 0; i < oldInteractive.length; i++){
				oldInteractive[i].removeEventListener('click', this.handleRemoveFilterRuleClick);
			}
		}

		let excludeHTML = '<p class="filters-block__body__label t-l">Исключить:</p>';
		const excludeList = window.app.filters.exclude;
		for(let i = 0; i < excludeList.length; i++){
			excludeHTML += `<p class="filters-block__body__data filters-block__data__removable t-l cu" type="exclude">${excludeList[i]}</p>`
		}

		let includeHTML = '<p class="filters-block__body__label t-l">Только:</p>';
		const includeList = window.app.filters.include;
		for(let i = 0; i < includeList.length; i++){
			includeHTML += `<p class="filters-block__body__data filters-block__data__removable t-l cu" type="include">${includeList[i]}</p>`
		}

		this.handleInputFilterType(window.app.filters.onlyType);

		excludeBody.innerHTML = excludeHTML;
		includeBody.innerHTML = includeHTML;

		const newInteractive = document.getElementsByClassName('filters-block__data__removable');
		for(let i = 0; i < newInteractive.length; i++){
			newInteractive[i].addEventListener('click', this.handleRemoveFilterRuleClick);
		}
	}
	
	handleAddExcludeClick = () => {
		const newExcludeFilter = excludeInput.value;
		if(newExcludeFilter === ''){
			alert('Введите значение для исключения');
			return;
		}
		window.app.filters.exclude.push(newExcludeFilter);
		this.displayUpdatedFilters();
		excludeInput.value = '';
	}
	handleAddIncludeClick = () => {
		const newIncludeFilter = includeInput.value;
		if(newIncludeFilter === ''){
			alert('Введите значение для белого списка');
			return;
		}
		window.app.filters.include.push(newIncludeFilter);
		this.displayUpdatedFilters();
		includeInput.value = '';
	}

	handleSubmitFiltersClick = () => {
		window.app.applyFilters();
		this.offOpenFiltersButton();
	}
	handleResetFiltersClick = () => {
		app.filters = {
			exclude: [],
			include: [],
			onlyType: 'all'
		}
		this.displayUpdatedFilters();
		window.app.resetFilteredResult();
		this.offOpenFiltersButton();
	}

	initEvents(){
		resultsBackButton.addEventListener('click', this.handleClickBack);
		resultsSaveButton.addEventListener('click', this.handleClickSave);
		resultsCopyButton.addEventListener('click', this.handleClickCopy);
		openFiltersButton.addEventListener('click', this.handleOpenFiltersClick);
		addExcludeButton.addEventListener('click', this.handleAddExcludeClick);
		addIncludeButton.addEventListener('click', this.handleAddIncludeClick);
		typeInputs.all.addEventListener('click', () => {this.handleInputFilterType('all')});
		typeInputs.file.addEventListener('click', () => {this.handleInputFilterType('file')});
		typeInputs.dir.addEventListener('click', () => {this.handleInputFilterType('dir')});
		filtersSubmitButton.addEventListener('click', this.handleSubmitFiltersClick);
		filtersResetButton.addEventListener('click', this.handleResetFiltersClick);
	}

	init(){
		workspaceWindow.classList.add('window-results__content_active-result');
		this.initEvents();
	}

}
window.workspaceController = new WorkSpaceController();