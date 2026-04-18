class App {

    constructor() {
        this.result_sys = null;
        this.result_filtered = null;
        this.filters = {
            exclude: [],
            include: [],
            onlyType: 'all'
        }
    }

    removeFilterValue(filterObj){
        const targetFilter = filterObj.type === 'exclude' ? this.filters.exclude : this.filters.include;
        const index = targetFilter.indexOf(filterObj.value);
        if(index === -1){
            this.crash('Вызван запрос на удаление фильтра, но фильтр не найден')
        }
        targetFilter.splice(index, 1);
    }

    resetFilteredResult(){
        this.result_filtered = null;
        window.uiController.renderResult(this.result_sys);
    }

    updateFilteredResult(filtered_result){
        this.result_filtered = filtered_result;
        window.uiController.renderResult(this.result_filtered);
    }

    applyFilters(){
        window.filtersEngine.execute();
    }

    collapse() {
        Neutralino.window.minimize();
    }
    close() {
        Neutralino.app.exit();
    }
    crash(message) {
        Neutralino.os.showMessageBox("Something went wrong", message, "OK", "ERROR");
        this.close();
    }

    saveResult(result_sys) {
        this.result_sys = result_sys;
    }

    async startSearch(startDir, searchStr){
        window.uiController.showLoading();
        let buffer = '';
        let ended = false;

        const start = performance.now();
        const proc = await Neutralino.os.spawnProcess(`"${NL_PATH}/beaver.exe" "${startDir}" "${searchStr}"`);
        
        Neutralino.events.on("spawnedProcess", (e) => {
            if (proc.id === e.detail.id) {
                switch (e.detail.action) {
                    case 'stdOut': {
                        const data = e.detail.data;
                        if (data.includes('<3>')) {
                            const end = performance.now();
                            // console.log(`Performance: ${end-start}ms`);
                            ended = true;
                            buffer += data;
                            const result = buffer.slice(0, -3).split('<2>');
                            window.uiController.showFoundResults(result);
                        }
                        else {
                            buffer += data;
                        }
                        break;
                    }
                    case 'stdErr': {
                        this.crash(`Something went wrong check beaver.exe for existence`);
                        break;
                    }
                    case 'exit': {
                        const code = e.detail.data;
                        if (code > 1) {
                            this.crash(`Memory allocation error on line: ${code}`);
                        }
                        else{
                            if(!ended){
                                this.crash(`Util exit with no end token`);
                            }
                        }
                        break;
                    }
                }
            }
        })
    }

    readArgv(){
        const args = NL_ARGS;
        if(args[1] === '--startDir'){
            const startDir = args[2].replace("\"", '');
            console.log(startDir);
            window.uiController.applyStartDir(startDir);
        }
    }

    async init() {
        await Neutralino.init();
        window.workspaceController.init();
        window.uiController.init();
        this.readArgv();
    }
}
window.app = new App();
window.app.init();