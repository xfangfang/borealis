async function main() {
    return {
        mainText: "borealis",
        secondaryText: 'borealis demo',
        onclick: async () => {
            return {
                path: window.workingDir + '/eboot',
                cwd: window.workingDir
            };
        }
    };
}
